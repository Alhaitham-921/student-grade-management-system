#include "maintenance.h"
#include <ctype.h>
#include <errno.h>
#include <io.h>
#include <math.h>
#include <stdint.h>
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

char userDisplayNames[MAX_USERS][60];
int classInactive[MAX_CLASSES];
typedef struct
{
    User users[MAX_USERS];
    Student students[MAX_STUDENTS];
    SchoolClass classes[MAX_CLASSES];
    char names[MAX_USERS][60];
    int inactive[MAX_CLASSES];
    int nu, ns, nc;
} State;
static State baseline;
static int baselineReady, ioOK = 1, auditOverflow;
static char activeDir[MAX_PATH], pendingAudit[32768];
static HANDLE directoryLock = INVALID_HANDLE_VALUE;
static const char *files[] = {"users.dat", "students.dat", "classes.dat", "metadata.dat", "audit.dat"};
static void capture(State *s)
{
    memcpy(s->users, users, sizeof(users));
    memcpy(s->students, students, sizeof(students));
    memcpy(s->classes, schoolClasses, sizeof(schoolClasses));
    memcpy(s->names, userDisplayNames, sizeof(userDisplayNames));
    memcpy(s->inactive, classInactive, sizeof(classInactive));
    s->nu = userCount;
    s->ns = studentCount;
    s->nc = classCount;
}
static void restore(void)
{
    memcpy(users, baseline.users, sizeof(users));
    memcpy(students, baseline.students, sizeof(students));
    memcpy(schoolClasses, baseline.classes, sizeof(schoolClasses));
    memcpy(userDisplayNames, baseline.names, sizeof(userDisplayNames));
    memcpy(classInactive, baseline.inactive, sizeof(classInactive));
    userCount = baseline.nu;
    studentCount = baseline.ns;
    classCount = baseline.nc;
    for (int i = 0; i < userCount; i++)
        if (!strcmp(users[i].username, currentUser.username))
            currentUser = users[i];
}
void storageBaseline(void)
{
    capture(&baseline);
    baselineReady = 1;
}
static void report(const char *path)
{
    DWORD code = GetLastError();
    int cerror = errno;
    wchar_t relative[MAX_PATH], absolute[32768];
    char utf8[4096];
    const char *shown = path;
    if (MultiByteToWideChar(CP_UTF8, 0, path, -1, relative, MAX_PATH) &&
        GetFullPathNameW(relative, 32768, absolute, NULL) &&
        WideCharToMultiByte(CP_UTF8, 0, absolute, -1, utf8, sizeof(utf8), NULL, NULL))
        shown = utf8;
    fprintf(stderr, "数据操作失败 path=%s win32=%lu errno=%d；未提交，已恢复上次状态。\n", shown, code, cerror);
}
int storageOpen(void)
{
    directoryLock =
        CreateFileW(L".sgms.lock", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (directoryLock == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "无法锁定数据目录，已有实例正在使用或权限不足 (win32=%lu)。\n", GetLastError());
        return 0;
    }
    return 1; /* OS closes the handle on normal exit or termination. */
}
static void put32(FILE *f, uint32_t v)
{
    unsigned char b[4] = {(unsigned char)v, (unsigned char)(v >> 8), (unsigned char)(v >> 16),
                          (unsigned char)(v >> 24)};
    if (fwrite(b, 1, 4, f) != 4)
        ioOK = 0;
}
static uint32_t get32(FILE *f)
{
    unsigned char b[4] = {0};
    if (fread(b, 1, 4, f) != 4)
        ioOK = 0;
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}
static void putText(FILE *f, const char *s, size_t cap)
{
    if (!memchr(s, 0, cap))
    {
        ioOK = 0;
        return;
    }
    uint32_t n = (uint32_t)strlen(s);
    put32(f, n);
    if (fwrite(s, 1, n, f) != n)
        ioOK = 0;
}
static void getText(FILE *f, char *s, size_t cap)
{
    uint32_t n = get32(f);
    if (n >= cap)
    {
        ioOK = 0;
        return;
    }
    if (fread(s, 1, n, f) != n)
        ioOK = 0;
    s[n] = 0;
    if (strlen(s) != n)
        ioOK = 0;
}
static void putFloat(FILE *f, float v)
{
    uint32_t bits;
    memcpy(&bits, &v, 4);
    put32(f, bits);
}
static float getFloat(FILE *f)
{
    uint32_t bits = get32(f);
    float v;
    memcpy(&v, &bits, 4);
    return v;
}
static uint32_t checksum(const char *path)
{
    FILE *f = fopen(path, "rb");
    uint32_t h = 2166136261u;
    int c;
    if (!f)
    {
        ioOK = 0;
        return 0;
    }
    while ((c = fgetc(f)) != EOF)
        h = (h ^ (unsigned char)c) * 16777619u;
    if (ferror(f))
        ioOK = 0;
    fclose(f);
    return h;
}
static void header(FILE *f, int kind)
{
    put32(f, 0x534d4753);
    put32(f, 1);
    put32(f, (uint32_t)kind);
}
static int readHeader(FILE *f, int kind)
{
    uint32_t a = get32(f), b = get32(f), c = get32(f);
    return ioOK && a == 0x534d4753 && b == 1 && c == (uint32_t)kind;
}
static int finish(FILE *f)
{
    int ok = !ferror(f);
    if (fflush(f) || _commit(_fileno(f)))
        ok = 0;
    if (fclose(f))
        ok = 0;
    return ok && ioOK;
}
static void pathFor(char *p, const char *dir, int kind)
{
    snprintf(p, MAX_PATH, "%s/%s", dir, files[kind]);
}
static int writePart(const char *dir, int kind)
{
    char path[MAX_PATH];
    pathFor(path, dir, kind);
    FILE *f = fopen(path, "wb");
    if (!f)
    {
        report(path);
        return 0;
    }
    header(f, kind);
    if (kind == 0)
    {
        put32(f, userCount);
        for (int i = 0; i < userCount; i++)
        {
            putText(f, users[i].username, 20);
            putText(f, users[i].password, 128);
            put32(f, users[i].type);
            putText(f, users[i].teachCourse, 30);
        }
    }
    if (kind == 1)
    {
        put32(f, studentCount);
        for (int i = 0; i < studentCount; i++)
        {
            Student *s = &students[i];
            putText(f, s->stuNum, 20);
            putText(f, s->name, 20);
            put32(f, s->courseCount);
            for (int j = 0; j < s->courseCount; j++)
            {
                putText(f, s->courses[j].course, 30);
                putFloat(f, s->courses[j].score);
            }
            put32(f, s->teacherCount);
            for (int j = 0; j < s->teacherCount; j++)
                putText(f, s->teachers[j], 20);
        }
    }
    if (kind == 2)
    {
        put32(f, classCount);
        for (int i = 0; i < classCount; i++)
        {
            SchoolClass *c = &schoolClasses[i];
            putText(f, c->name, CLASS_NAME_LEN);
            put32(f, c->count);
            for (int j = 0; j < c->count; j++)
                putText(f, c->members[j], 20);
        }
    }
    if (kind == 3)
    {
        put32(f, userCount);
        for (int i = 0; i < userCount; i++)
            putText(f, userDisplayNames[i], 60);
        put32(f, classCount);
        for (int i = 0; i < classCount; i++)
            put32(f, classInactive[i]);
    }
    return finish(f);
}
static int readPart(const char *dir, int kind)
{
    char path[MAX_PATH];
    pathFor(path, dir, kind);
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
    if (!readHeader(f, kind))
    {
        fclose(f);
        return 0;
    }
    uint32_t n = get32(f);
    if (kind == 0)
    {
        if (n > MAX_USERS)
            ioOK = 0;
        else
        {
            userCount = (int)n;
            for (int i = 0; i < userCount && ioOK; i++)
            {
                getText(f, users[i].username, 20);
                getText(f, users[i].password, 128);
                users[i].type = (UserType)get32(f);
                getText(f, users[i].teachCourse, 30);
            }
        }
    }
    if (kind == 1)
    {
        if (n > MAX_STUDENTS)
            ioOK = 0;
        else
        {
            studentCount = (int)n;
            for (int i = 0; i < studentCount && ioOK; i++)
            {
                Student *s = &students[i];
                getText(f, s->stuNum, 20);
                getText(f, s->name, 20);
                uint32_t m = get32(f);
                if (m > MAX_COURSES)
                {
                    ioOK = 0;
                    break;
                }
                s->courseCount = (int)m;
                for (int j = 0; j < s->courseCount && ioOK; j++)
                {
                    getText(f, s->courses[j].course, 30);
                    s->courses[j].score = getFloat(f);
                }
                m = get32(f);
                if (m > MAX_TEACHERS_PER_STUDENT)
                {
                    ioOK = 0;
                    break;
                }
                s->teacherCount = (int)m;
                for (int j = 0; j < s->teacherCount && ioOK; j++)
                    getText(f, s->teachers[j], 20);
            }
        }
    }
    if (kind == 2)
    {
        if (n > MAX_CLASSES)
            ioOK = 0;
        else
        {
            classCount = (int)n;
            for (int i = 0; i < classCount && ioOK; i++)
            {
                SchoolClass *c = &schoolClasses[i];
                getText(f, c->name, CLASS_NAME_LEN);
                uint32_t m = get32(f);
                if (m > MAX_STUDENTS)
                {
                    ioOK = 0;
                    break;
                }
                c->count = (int)m;
                for (int j = 0; j < c->count && ioOK; j++)
                    getText(f, c->members[j], 20);
            }
        }
    }
    if (kind == 3)
    {
        if (n != (uint32_t)userCount)
            ioOK = 0;
        else
            for (int i = 0; i < userCount && ioOK; i++)
                getText(f, userDisplayNames[i], 60);
        if (get32(f) != (uint32_t)classCount)
            ioOK = 0;
        else
            for (int i = 0; i < classCount && ioOK; i++)
            {
                classInactive[i] = (int)get32(f);
                if (classInactive[i] != 0 && classInactive[i] != 1)
                    ioOK = 0;
            }
    }
    int ok = ioOK && fgetc(f) == EOF && !ferror(f);
    fclose(f);
    return ok;
}
static int validState(void)
{
    int admin = 0;
    for (int i = 0; i < userCount; i++)
    {
        User *u = &users[i];
        if (!u->username[0] || u->type < ADMIN || u->type > STUDENT || strlen(u->password) != 99 ||
            strncmp(u->password, "p1$", 3))
            return 0;
        for (int k = 3; k < 99; k++)
            if (!isxdigit((unsigned char)u->password[k]))
                return 0;
        for (int j = 0; j < i; j++)
            if (!strcmp(u->username, users[j].username))
                return 0;
        if (u->type == ADMIN)
            admin = 1;
    }
    if (userCount && !admin)
        return 0;
    for (int i = 0; i < studentCount; i++)
    {
        Student *s = &students[i];
        if (!s->stuNum[0] || !s->name[0])
            return 0;
        int account = 0;
        for (int j = 0; j < userCount; j++)
            if (users[j].type == STUDENT && !strcmp(users[j].username, s->stuNum))
                account = 1;
        if (!account)
            return 0;
        for (int j = 0; j < i; j++)
            if (!strcmp(s->stuNum, students[j].stuNum))
                return 0;
        for (int j = 0; j < s->courseCount; j++)
        {
            if (!s->courses[j].course[0] || !isfinite(s->courses[j].score) || s->courses[j].score < 0 ||
                s->courses[j].score > 100)
                return 0;
            for (int k = 0; k < j; k++)
                if (!strcmp(s->courses[k].course, s->courses[j].course))
                    return 0;
        }
        for (int j = 0; j < s->teacherCount; j++)
        {
            int found = 0;
            for (int k = 0; k < userCount; k++)
                if (users[k].type == TEACHER && !strcmp(users[k].username, s->teachers[j]))
                    found = 1;
            if (!found)
                return 0;
            for (int k = 0; k < j; k++)
                if (!strcmp(s->teachers[k], s->teachers[j]))
                    return 0;
        }
    }
    for (int i = 0; i < classCount; i++)
    {
        SchoolClass *c = &schoolClasses[i];
        if (!c->name[0])
            return 0;
        for (int k = 0; k < i; k++)
            if (!strcmp(c->name, schoolClasses[k].name))
                return 0;
        for (int j = 0; j < c->count; j++)
        {
            if (findStudentIndex(c->members[j]) < 0)
                return 0;
            for (int k = 0; k <= i; k++)
                for (int m = 0; m < (k == i ? j : schoolClasses[k].count); m++)
                    if (!strcmp(c->members[j], schoolClasses[k].members[m]))
                        return 0;
        }
    }
    return 1;
}
int storageLoad(void)
{
    FILE *f = fopen(".sgms/current.dat", "rb");
    if (!f)
    {
        if (errno == ENOENT)
            return 0;
        report(".sgms/current.dat");
        exit(1);
    }
    char id[49] = {0}, path[MAX_PATH];
    uint32_t sums[5];
    ioOK = 1;
    if (!readHeader(f, 99))
        ioOK = 0;
    getText(f, id, sizeof(id));
    for (int i = 0; i < 5; i++)
        sums[i] = get32(f);
    if (fgetc(f) != EOF || ferror(f))
        ioOK = 0;
    fclose(f);
    if (!id[0])
        ioOK = 0;
    for (size_t i = 0; id[i]; i++)
        if (!isalnum((unsigned char)id[i]) && id[i] != '-')
            ioOK = 0;
    snprintf(activeDir, sizeof(activeDir), ".sgms/%s", id);
    for (int i = 0; i < 5 && ioOK; i++)
    {
        pathFor(path, activeDir, i);
        if (checksum(path) != sums[i])
            ioOK = 0;
    }
    for (int i = 0; i < 4 && ioOK; i++)
        if (!readPart(activeDir, i))
            ioOK = 0;
    if (!ioOK || !validState())
    {
        fprintf(stderr, "当前数据集损坏或版本不兼容；拒绝加载，未覆盖任何业务数据。\n");
        exit(1);
    }
    refreshAverages();
    storageBaseline();
    return 1;
}
void storageBackupLegacy(void)
{
    int exists = 0;
    for (int i = 0; i < 3; i++)
        if (GetFileAttributesA(files[i]) != INVALID_FILE_ATTRIBUTES)
            exists = 1;
    if (!exists)
        return;
    unsigned char random[8] = {0};
    if (BCryptGenRandom(NULL, random, 8, BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
    {
        fprintf(stderr, "无法创建唯一迁移备份；未覆盖原文件。\n");
        exit(EXIT_FAILURE);
    }
    char dir[MAX_PATH], dst[MAX_PATH];
    snprintf(dir, sizeof(dir), "legacy-backup-%02x%02x%02x%02x%02x%02x%02x%02x", random[0], random[1], random[2],
             random[3], random[4], random[5], random[6], random[7]);
    if (!CreateDirectoryA(dir, NULL))
    {
        report(dir);
        exit(1);
    }
    for (int i = 0; i < 3; i++)
        if (GetFileAttributesA(files[i]) != INVALID_FILE_ATTRIBUTES)
        {
            pathFor(dst, dir, i);
            if (!CopyFileA(files[i], dst, TRUE))
            {
                report(dst);
                exit(1);
            }
        }
    printf("升级前完整副本: %s\n", dir);
}
static void cleanLog(char *s)
{
    for (; *s; s++)
        if ((unsigned char)*s < 32 || *s == 127)
            *s = ' ';
}
void auditEvent(const char *action, const char *detail)
{
    SYSTEMTIME t;
    GetLocalTime(&t);
    char who[64], what[2048], line[2300];
    snprintf(who, sizeof(who), "%s", currentUser.username[0] ? currentUser.username : "system");
    snprintf(what, sizeof(what), "%s", detail);
    cleanLog(who);
    cleanLog(what);
    snprintf(line, sizeof(line), "%04d-%02d-%02d %02d:%02d:%02d\t%s\t%s\t%s\n", t.wYear, t.wMonth, t.wDay, t.wHour,
             t.wMinute, t.wSecond, who, action, what);
    if (strlen(pendingAudit) + strlen(line) < sizeof(pendingAudit))
        strcat(pendingAudit, line);
    else
    {
        ioOK = 0;
        auditOverflow = 1;
    }
}
static void auditChanges(void)
{
    char detail[1024];
    for (int i = 0; i < studentCount; i++)
    {
        Student *s = &students[i];
        Student *old = NULL;
        for (int j = 0; j < baseline.ns; j++)
            if (!strcmp(s->stuNum, baseline.students[j].stuNum))
                old = &baseline.students[j];
        for (int j = 0; j < s->courseCount; j++)
        {
            int k = old ? courseIndex(old, s->courses[j].course) : -1;
            if (k < 0 || old->courses[k].score != s->courses[j].score)
            {
                char before[32] = "--";
                if (k >= 0)
                    snprintf(before, sizeof(before), "%.9g", old->courses[k].score);
                snprintf(detail, sizeof(detail), "student=%s course=%s before=%s after=%.9g", s->stuNum,
                         s->courses[j].course, before, s->courses[j].score);
                auditEvent("score", detail);
            }
        }
        int before = -1, after = studentClass(s->stuNum);
        for (int c = 0; c < baseline.nc; c++)
            for (int j = 0; j < baseline.classes[c].count; j++)
                if (!strcmp(s->stuNum, baseline.classes[c].members[j]))
                    before = c;
        if (before != after)
        {
            snprintf(detail, sizeof(detail), "student=%s from=%s to=%s", s->stuNum,
                     before < 0 ? "--" : baseline.classes[before].name, after < 0 ? "--" : schoolClasses[after].name);
            auditEvent("class-transfer", detail);
        }
    }
}
static int writeAudit(const char *dir)
{
    char p[MAX_PATH];
    pathFor(p, dir, 4);
    FILE *out = fopen(p, "wb");
    if (!out)
        return 0;
    if (activeDir[0])
    {
        pathFor(p, activeDir, 4);
        FILE *in = fopen(p, "rb");
        if (!in)
        {
            fclose(out);
            return 0;
        }
        char b[4096];
        size_t n;
        while ((n = fread(b, 1, sizeof(b), in)) > 0)
            if (fwrite(b, 1, n, out) != n)
                ioOK = 0;
        if (ferror(in))
            ioOK = 0;
        fclose(in);
    }
    if (fwrite(pendingAudit, 1, strlen(pendingAudit), out) != strlen(pendingAudit))
        ioOK = 0;
    return finish(out);
}
int storageCommit(void)
{
    ioOK = !auditOverflow;
    refreshAverages();
    if (!validState())
    {
        fprintf(stderr, "数据关联校验失败，拒绝保存。\n");
        if (baselineReady)
            restore();
        pendingAudit[0] = 0;
        auditOverflow = 0;
        return 0;
    }
    if (baselineReady)
        auditChanges();
    unsigned char r[16] = {0};
    if (BCryptGenRandom(NULL, r, 16, BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
        ioOK = 0;
    char id[40], dir[MAX_PATH], p[MAX_PATH];
    for (int i = 0; i < 16; i++)
        sprintf(id + i * 2, "%02x", r[i]);
    snprintf(dir, sizeof(dir), ".sgms/%s", id);
    if (!CreateDirectoryA(".sgms", NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        ioOK = 0;
    if (ioOK && !CreateDirectoryA(dir, NULL))
        ioOK = 0;
    uint32_t sums[5] = {0};
    for (int i = 0; i < 4 && ioOK; i++)
    {
        if (!writePart(dir, i))
            ioOK = 0;
        pathFor(p, dir, i);
        sums[i] = checksum(p);
    }
    if (ioOK && !writeAudit(dir))
        ioOK = 0;
    pathFor(p, dir, 4);
    if (ioOK)
        sums[4] = checksum(p);
    /* Read back the portable records before publishing their generation. */
    for (int i = 0; i < 4 && ioOK; i++)
        if (!readPart(dir, i))
            ioOK = 0;
    if (ioOK && !validState())
        ioOK = 0;
    refreshAverages();
    /* Fault injection is available only in the explicitly built test executable. */
#ifdef SGMS_TESTING
    static int commits = 0;
    const char *fault = getenv("SGMS_FAULT");
    const char *after = getenv("SGMS_FAULT_AFTER");
    if (++commits <= (after ? atoi(after) : 0))
        fault = NULL;
    if (fault && !strcmp(fault, "before-marker"))
    {
        ioOK = 0;
        SetLastError(ERROR_WRITE_FAULT);
        errno = EIO;
    }
    if (fault && !strcmp(fault, "crash-before-marker"))
        ExitProcess(86);
#endif
    if (ioOK)
    {
        FILE *f = fopen(".sgms/pending.tmp", "wb");
        if (!f)
            ioOK = 0;
        else
        {
            header(f, 99);
            putText(f, id, sizeof(id));
            for (int i = 0; i < 5; i++)
                put32(f, sums[i]);
            if (!finish(f))
                ioOK = 0;
        }
    }
    if (ioOK &&
        !MoveFileExA(".sgms/pending.tmp", ".sgms/current.dat", MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        ioOK = 0;
    if (!ioOK)
    {
        report(dir);
        if (baselineReady)
            restore();
        pendingAudit[0] = 0;
        auditOverflow = 0;
        return 0;
    }
#ifdef SGMS_TESTING
    if (fault && !strcmp(fault, "crash-after-marker"))
        ExitProcess(87);
#endif
    strcpy(activeDir, dir);
    storageBaseline();
    pendingAudit[0] = 0;
    auditOverflow = 0;
    return 1;
}
void auditLogin(const char *username, int success)
{
    char detail[100];
    snprintf(detail, sizeof(detail), "login=%s result=%s", username, success ? "success" : "failure");
    User previous = currentUser;
    if (!success)
        strcpy(currentUser.username, "anonymous");
    auditEvent("login", detail);
    storageCommit();
    currentUser = previous;
}
void showAudit(void)
{
    if (currentUser.type != ADMIN)
    {
        printf("无权查看日志。\n");
        return;
    }
    char p[MAX_PATH];
    pathFor(p, activeDir, 4);
    FILE *f = fopen(p, "rb");
    if (!f)
    {
        report(p);
        return;
    }
    char line[4096];
    int n = 0;
    while (fgets(line, sizeof(line), f))
    {
        fputs(line, stdout);
        if (!pageNext(++n))
            break;
    }
    fclose(f);
}
