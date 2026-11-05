#include "maintenance.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <math.h>
#include <sys/stat.h>
#include <windows.h>

/* Streaming RFC-style fields, including escaped quotes and embedded CR/LF.
   Return -1 for a malformed record, 0 for EOF, 1 for a record. */
static int readCSV(FILE *f, char fields[4][512], int *physical, int *columns)
{
    int field = 0, len = 0, state = 0, bad = 0, any = 0, ch;
    memset(fields, 0, 4 * 512);
    while ((ch = fgetc(f)) != EOF)
    {
        any = 1;
        if (ch == 0)
            bad = 1;
        if (ch == '\r')
        {
            int next = fgetc(f);
            if (next != '\n' && next != EOF)
                ungetc(next, f);
            ch = '\n';
        }
        if (ch == '\n')
            (*physical)++;
        if (state == 2)
        {
            if (ch == '"')
                state = 3;
            else
            {
                if (field < 4 && len < 511)
                    fields[field][len++] = (char)ch;
                else
                    bad = 1;
            }
            continue;
        }
        if (state == 3 && ch == '"')
        {
            if (field < 4 && len < 511)
                fields[field][len++] = '"';
            else
                bad = 1;
            state = 2;
            continue;
        }
        if (ch == ',' || ch == '\n')
        {
            if (field < 4)
                fields[field][len] = 0;
            if (ch == '\n')
            {
                if (!bad && field == 0 && len == 0 && state == 0)
                    return 2;
                *columns = field + 1;
                return !bad && (field == 2 || field == 3) ? 1 : -1;
            }
            field++;
            len = 0;
            state = 0;
            continue;
        }
        if (state == 3)
        {
            bad = 1;
            continue;
        }
        if (ch == '"')
        {
            if (state == 0)
                state = 2;
            else
                bad = 1;
            continue;
        }
        state = 1;
        if (field < 4 && len < 511)
            fields[field][len++] = (char)ch;
        else
            bad = 1;
    }
    if (!any)
        return 0;
    if (field < 4)
        fields[field][len] = 0;
    *columns = field + 1;
    return !bad && state != 2 && (field == 2 || field == 3) ? 1 : -1;
}
static FILE *inputPath(char *path, size_t n, const wchar_t *mode)
{
    if (safeReadString(path, n, "CSV路径 (:q取消，不加外层引号): ") != INPUT_OK)
        return NULL;
    wchar_t wide[512];
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, 512))
    {
        printf("路径编码无效。\n");
        return NULL;
    }
    FILE *f = _wfopen(wide, mode);
    if (!f)
        printf("无法打开CSV path=%s errno=%d win32=%lu\n", path, errno, GetLastError());
    return f;
}
void csvImport(void)
{
    if (currentUser.type != TEACHER)
    {
        printf("无权导入。\n");
        return;
    }
    int c = selectClass();
    if (c < 0)
        return;
    if (classInactive[c])
    {
        printf("班级已停用，不能导入新成绩。\n");
        return;
    }
    char path[512];
    FILE *f = inputPath(path, sizeof(path), L"rb");
    if (!f)
        return;
    if (_filelengthi64(_fileno(f)) > 8 * 1024 * 1024)
    {
        fclose(f);
        printf("CSV超过8MiB。\n");
        return;
    }
    unsigned char bom[3];
    size_t got = fread(bom, 1, 3, f);
    if (got != 3 || memcmp(bom, "\xef\xbb\xbf", 3))
        rewind(f);
    Student *staged = malloc(sizeof(students));
    if (!staged)
    {
        fclose(f);
        printf("内存不足。\n");
        return;
    }
    memcpy(staged, students, sizeof(students));
    int physical = 1, records = 0, errors = 0, imported = 0, seen[MAX_STUDENTS] = {0};
    char fields[4][512];
    int rc, columns = 0, expectedColumns = 0;
    while (1)
    {
        int line = physical;
        rc = readCSV(f, fields, &physical, &columns);
        if (!rc)
            break;
        if (rc == 2)
            continue;
        records++;
        const char *reason = NULL;
        if (rc < 0)
            reason = "CSV字段数、长度或引号格式错误";
        if (!reason)
        {
            if (!expectedColumns)
                expectedColumns = columns;
            if (columns != expectedColumns)
                reason = "列数与表头或首条数据不一致";
        }
        int courseColumn = columns == 4 ? 2 : 1, scoreColumn = columns == 4 ? 3 : 2;
        if (!reason && records == 1 &&
            ((!strcmp(fields[0], "stuNum") && !strcmp(fields[courseColumn], "course") &&
              !strcmp(fields[scoreColumn], "score") && (columns == 3 || !strcmp(fields[1], "name"))) ||
             (!strcmp(fields[0], "学号") && !strcmp(fields[courseColumn], "科目") &&
              !strcmp(fields[scoreColumn], "成绩") && (columns == 3 || !strcmp(fields[1], "姓名")))))
            continue;
        int i = -1, j = -1;
        float v = 0;
        char *end = NULL;
        if (!reason)
        {
            errno = 0;
            v = strtof(fields[scoreColumn], &end);
            int parsed = end != fields[scoreColumn];
            while (end && isspace((unsigned char)*end))
                end++;
            if (!parsed || *end || errno || !isfinite(v) || v < 0 || v > 100)
                reason = "成绩必须为0到100的有限数字";
            else if (!validStudentId(fields[0]))
                reason = "学号必须为数字字符串";
            else if ((i = findStudentIndex(fields[0])) < 0)
                reason = "学号不存在";
            else if (columns == 4 && strcmp(fields[1], students[i].name))
                reason = "姓名与学号不匹配";
            else if (studentClass(fields[0]) != c)
                reason = "学生不属于所选班级";
            else if (strcmp(fields[courseColumn], currentUser.teachCourse))
                reason = "科目不匹配";
            else if (!hasTeacherAccess(&students[i]))
                reason = "无授课权限";
            else if (seen[i])
                reason = "文件内重复成绩";
            else if ((j = courseIndex(&staged[i], fields[courseColumn])) >= 0)
                reason = "该课程已有成绩";
            else if (staged[i].courseCount >= MAX_COURSES)
                reason = "课程数量已达上限";
        }
        if (reason)
        {
            printf("第%d行: %s\n", line, reason);
            errors++;
            continue;
        }
        seen[i] = 1;
        j = staged[i].courseCount++;
        strcpy(staged[i].courses[j].course, fields[courseColumn]);
        staged[i].courses[j].score = v;
        imported++;
    }
    if (ferror(f))
    {
        printf("CSV读取失败。\n");
        errors++;
    }
    fclose(f);
    if (errors)
    {
        printf("共%d处错误，本次未导入任何成绩。\n", errors);
        free(staged);
        return;
    }
    if (inputFinished())
    {
        free(staged);
        return;
    }
    if (!imported)
    {
        printf("文件中没有可导入的成绩。\n");
        free(staged);
        return;
    }
    printf("导入预览 班级: %s 科目: %s 待导入: %d\n", schoolClasses[c].name, currentUser.teachCourse, imported);
    const int w[] = {20, 16, 24, 10};
    const char *h[] = {"学号", "姓名", "科目", "成绩"};
    tableStart(4, w, h);
    for (int i = 0; i < studentCount; i++)
        if (seen[i])
        {
            int j = courseIndex(&staged[i], currentUser.teachCourse);
            tableRow("sssf", staged[i].stuNum, staged[i].name, currentUser.teachCourse,
                     (double)staged[i].courses[j].score);
        }
    tableEnd();
    if (!confirmAction("1确认导入 / 0取消: "))
    {
        free(staged);
        printf("已取消导入。\n");
        return;
    }
    memcpy(students, staged, sizeof(students));
    free(staged);
    char detail[256];
    snprintf(detail, sizeof(detail), "class=%s course=%s rows=%d", schoolClasses[c].name, currentUser.teachCourse,
             imported);
    auditEvent("csv-import", detail);
    if (saveAll())
        printf("成功导入 %d 条成绩。\n", imported);
}
static void writeField(FILE *f, const char *text)
{
    fputc('"', f);
    for (; *text; text++)
    {
        if (*text == '"')
            fputc('"', f);
        fputc(*text, f);
    }
    fputc('"', f);
}
void csvExport(void)
{
    if (currentUser.type != TEACHER && currentUser.type != ADMIN)
    {
        printf("无权导出。\n");
        return;
    }
    int c = selectClass();
    if (c < 0)
        return;
    char course[30];
    if (!selectCourse(course, sizeof(course)))
        return;
    char path[512];
    if (safeReadString(path, sizeof(path), "导出CSV新文件路径 (:q取消): ") != INPUT_OK)
        return;
    wchar_t wide[512];
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, 512))
    {
        printf("路径编码无效。\n");
        return;
    }
    int fd = _wopen(wide, _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY, _S_IREAD | _S_IWRITE);
    if (fd < 0)
    {
        printf("无法创建导出文件（路径可能已存在）errno=%d。\n", errno);
        return;
    }
    FILE *f = _fdopen(fd, "wb");
    if (!f)
    {
        _close(fd);
        printf("导出失败。\n");
        return;
    }
    fputs("\xef\xbb\xbfstuNum,course,score\r\n", f);
    int n = 0;
    for (int i = 0; i < studentCount; i++)
        if (studentClass(students[i].stuNum) == c && hasTeacherAccess(&students[i]))
        {
            int j = courseIndex(&students[i], course);
            if (j < 0)
                continue;
            writeField(f, students[i].stuNum);
            fputc(',', f);
            writeField(f, course);
            fprintf(f, ",%.9g\r\n", students[i].courses[j].score);
            n++;
        }
    int ok = !ferror(f);
    if (fflush(f) || _commit(_fileno(f)))
        ok = 0;
    if (fclose(f))
        ok = 0;
    if (!ok)
    {
        printf("导出失败；保留部分文件供检查，不可视为完整导出。\n");
        return;
    }
    char detail[256];
    snprintf(detail, sizeof(detail), "class=%s course=%s rows=%d", schoolClasses[c].name, course, n);
    auditEvent("csv-export", detail);
    if (saveAll())
        printf("成功导出 %d 条成绩。\n", n);
    else
        printf("导出文件已生成，但日志未提交。\n");
}
