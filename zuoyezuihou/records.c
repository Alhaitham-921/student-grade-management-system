#include "maintenance.h"
#include <io.h>

int courseIndex(const Student *st, const char *course)
{
    for (int i = 0; i < st->courseCount; i++)
        if (!strcmp(st->courses[i].course, course))
            return i;
    return -1;
}
int hasTeacherAccess(const Student *st)
{
    if (currentUser.type == ADMIN)
        return 1;
    if (currentUser.type != TEACHER)
        return 0;
    for (int i = 0; i < st->teacherCount; i++)
        if (!strcmp(st->teachers[i], currentUser.username))
            return 1;
    return 0;
}
const char *displayName(const char *login)
{
    for (int i = 0; i < userCount; i++)
        if (!strcmp(users[i].username, login))
            return userDisplayNames[i][0] ? userDisplayNames[i] : login;
    return login;
}
int pageNext(int count)
{
    if (count % 10 || !_isatty(_fileno(stdin)))
        return 1;
    return safeReadInt("1继续 / 0返回: ", 0, 1);
}
void editRecords(void)
{
    if (currentUser.type != ADMIN)
    {
        printf("无权修改档案。\n");
        return;
    }
    int op = safeReadInt("1学生姓名 / 2教师显示姓名 / 3授课关系 / 0取消: ", 0, 3);
    if (!op)
        return;
    char id[20];
    safeReadString(id, sizeof(id), op == 2 ? "教师登录名 (0取消): " : "学号 (0取消): ");
    if (!strcmp(id, "0"))
        return;
    if (op == 2)
    {
        int u = -1;
        for (int i = 0; i < userCount; i++)
            if (users[i].type == TEACHER && !strcmp(users[i].username, id))
                u = i;
        if (u < 0)
        {
            printf("教师不存在。\n");
            return;
        }
        safeReadString(userDisplayNames[u], 60, "显示姓名: ");
        auditEvent("teacher-display-name", id);
    }
    else
    {
        int s = findStudentIndex(id);
        if (s < 0)
        {
            printf("学生不存在。\n");
            return;
        }
        if (op == 1)
        {
            safeReadString(students[s].name, 20, "学生姓名: ");
            auditEvent("student-name", id);
        }
        else
        {
            char chosen[MAX_TEACHERS_PER_STUDENT][20];
            int count = 0, indices[MAX_USERS], n = 0;
            const int w[] = {6, 20, 24, 24};
            const char *h[] = {"序号", "登录名", "显示姓名", "科目"};
            tableStart(4, w, h);
            for (int i = 0; i < userCount; i++)
                if (users[i].type == TEACHER)
                {
                    indices[n++] = i;
                    tableRow("dsss", n, users[i].username, displayName(users[i].username), users[i].teachCourse);
                }
            tableEnd();
            while (count < MAX_TEACHERS_PER_STUDENT)
            {
                int k = safeReadInt("选择教师 (0完成): ", 0, n) - 1;
                if (k < 0)
                    break;
                int u = indices[k], dup = 0;
                for (int j = 0; j < count; j++)
                    if (!strcmp(chosen[j], users[u].username))
                        dup = 1;
                if (dup)
                {
                    printf("该教师已选择。\n");
                    continue;
                }
                strcpy(chosen[count++], users[u].username);
            }
            if (!count)
            {
                printf("未选择教师，取消修改。\n");
                return;
            }
            memset(students[s].teachers, 0, sizeof(students[s].teachers));
            memcpy(students[s].teachers, chosen, (size_t)count * 20);
            students[s].teacherCount = count;
            auditEvent("teacher-assignment", id);
        }
    }
    if (saveAll())
        printf("档案更新成功。\n");
}
static void renderList(int scores, int filter, const char *key)
{
    int count = 0;
    const int w[] = {6, 20, 16, 24, 20, 10};
    const char *hs[] = {"序号", "学号", "姓名", "班级", "科目数", "平均分"};
    const char *hg[] = {"序号", "学号", "姓名", "班级", "科目", "成绩"};
    tableStart(6, w, scores ? hg : hs);
    for (int i = 0; i < studentCount; i++)
    {
        Student *s = &students[i];
        int c = studentClass(s->stuNum);
        if (!hasTeacherAccess(s) || (filter >= 0 && c != filter))
            continue;
        if (key && key[0] && !strstr(s->stuNum, key) && !strstr(s->name, key))
            continue;
        const char *name = c < 0 ? "未选择" : schoolClasses[c].name;
        int rows = scores && s->courseCount ? s->courseCount : 1;
        for (int j = 0; j < rows; j++)
        {
            if (!scores)
            {
                char avg[32] = "--";
                if (s->courseCount)
                    snprintf(avg, sizeof(avg), "%.1f", s->average);
                tableRow("dsssds", ++count, s->stuNum, s->name, name, s->courseCount, avg);
            }
            else if (!s->courseCount)
                tableRow("dsssss", ++count, s->stuNum, s->name, name, "暂无成绩", "--");
            else
                tableRow("dssssf", ++count, s->stuNum, s->name, name, s->courses[j].course,
                         (double)s->courses[j].score);
            if (count % 10 == 0)
            {
                tableEnd();
                if (!pageNext(count))
                    return;
                tableStart(6, w, scores ? hg : hs);
            }
        }
    }
    tableEnd();
    printf("共 %d 条记录。\n", count);
}
void filteredList(int scores)
{
    if (currentUser.type != ADMIN && currentUser.type != TEACHER)
    {
        printf("无权查看。\n");
        return;
    }
    int mode = safeReadInt("1全部 / 2按班级 / 0取消: ", 0, 2), c = -1;
    if (!mode)
        return;
    if (mode == 2)
    {
        c = selectClass();
        if (c < 0)
            return;
    }
    renderList(scores, c, NULL);
}
void searchRecords(void)
{
    if (currentUser.type != ADMIN && currentUser.type != TEACHER)
        return;
    char key[60];
    safeReadString(key, sizeof(key), "学号/部分姓名 (0取消): ");
    if (!strcmp(key, "0"))
        return;
    int mode = safeReadInt("1所有班级 / 2选择班级 / 0取消: ", 0, 2), c = -1;
    if (!mode)
        return;
    if (mode == 2)
    {
        c = selectClass();
        if (c < 0)
            return;
    }
    renderList(0, c, key);
}
