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
            return userDisplayNames[i][0] ? userDisplayNames[i] : users[i].type == TEACHER ? "待完善" : login;
    return login;
}
int pageNext(int count)
{
    if (count % 10 || !_isatty(_fileno(stdin)))
        return 1;
    return safeReadInt("1继续 / 2返回: ", 1, 2) == 1;
}
OperationResult createTeacher(const User *u, const char *name)
{
    if (currentUser.type != ADMIN)
        return RESULT_DENIED;
    if (userCount >= MAX_USERS || u->type != TEACHER || !validTeacherId(u->username) || !validPersonName(name) ||
        strlen(name) >= 60 || !u->teachCourse[0] || userExists(u->username))
        return RESULT_INVALID;
    users[userCount] = *u;
    strcpy(userDisplayNames[userCount++], name);
    auditEvent("teacher-create", u->username);
    return saveAll() ? RESULT_OK : RESULT_IO;
}
OperationResult createStudent(const Student *s, int c)
{
    if (currentUser.type != ADMIN)
        return RESULT_DENIED;
    if (studentCount >= MAX_STUDENTS || userCount >= MAX_USERS || !validStudentId(s->stuNum) ||
        !validPersonName(s->name) || findStudentIndex(s->stuNum) >= 0 || userExists(s->stuNum) || c < 0 ||
        c >= classCount || classInactive[c] || schoolClasses[c].count >= MAX_STUDENTS || s->teacherCount < 1 ||
        s->teacherCount > MAX_TEACHERS_PER_STUDENT)
        return RESULT_INVALID;
    for (int j = 0; j < s->teacherCount; j++)
    {
        int found = 0;
        for (int u = 0; u < userCount; u++)
            if (users[u].type == TEACHER && !strcmp(users[u].username, s->teachers[j]))
                found = 1;
        if (!found)
            return RESULT_INVALID;
        for (int k = 0; k < j; k++)
            if (!strcmp(s->teachers[k], s->teachers[j]))
                return RESULT_INVALID;
    }
    User u = {0};
    strcpy(u.username, s->stuNum);
    u.type = STUDENT;
    hashPassword(s->stuNum, u.password);
    users[userCount] = u;
    strcpy(userDisplayNames[userCount++], s->name);
    students[studentCount++] = *s;
    strcpy(schoolClasses[c].members[schoolClasses[c].count++], s->stuNum);
    auditEvent("student-create", s->stuNum);
    return saveAll() ? RESULT_OK : RESULT_IO;
}
int selectTeachers(char chosen[MAX_TEACHERS_PER_STUDENT][20])
{
    int indices[MAX_USERS], n = 0, count = 0;
    const int w[] = {6, 20, 24, 24};
    const char *h[] = {"序号", "工号", "姓名", "科目"};
    tableStart(4, w, h);
    for (int u = 0; u < userCount; u++)
        if (users[u].type == TEACHER && validPersonName(userDisplayNames[u]))
        {
            indices[n++] = u;
            tableRow("dsss", n, users[u].username, displayName(users[u].username), users[u].teachCourse);
        }
    tableEnd();
    if (!n)
    {
        printf("暂无姓名完整的教师，请先添加教师或完善姓名。\n");
        return -1;
    }
    while (count < MAX_TEACHERS_PER_STUDENT)
    {
        int k = safeReadInt("教师序号（0完成，:q取消）: ", 0, n);
        if (k < 0)
            return -1;
        if (!k)
            break;
        int u = indices[k - 1], duplicate = 0;
        for (int j = 0; j < count; j++)
            if (!strcmp(chosen[j], users[u].username))
                duplicate = 1;
        if (duplicate)
        {
            printf("该教师已选择！\n");
            continue;
        }
        strcpy(chosen[count++], users[u].username);
    }
    if (!count)
        printf("至少需要选择一位教师，已取消。\n");
    return count;
}
static int pickStudent(void)
{
    char id[20];
    if (readStudentId(id, 20, "学生学号 (:q取消): ") != INPUT_OK)
        return -1;
    int s = findStudentIndex(id);
    if (s < 0)
        printf("学生不存在。\n");
    return s;
}
void editTeaching(void)
{
    if (currentUser.type != ADMIN)
        return;
    int s = pickStudent();
    if (s < 0)
        return;
    char chosen[MAX_TEACHERS_PER_STUDENT][20];
    int n = selectTeachers(chosen);
    if (n <= 0)
        return;
    if (!confirmAction("1确认授课关系 / 0取消: "))
        return;
    memset(students[s].teachers, 0, sizeof(students[s].teachers));
    memcpy(students[s].teachers, chosen, (size_t)n * 20);
    students[s].teacherCount = n;
    auditEvent("teacher-assignment", students[s].stuNum);
    if (saveAll())
        printf("授课关系更新成功。\n");
}
void editStudent(void)
{
    if (currentUser.type != ADMIN)
        return;
    int s = pickStudent();
    if (s < 0)
        return;
    const char *items[] = {"修改姓名", "修改授课关系", "返回上一级"};
    int op = menuChoice("管理员 / 学生管理 / 修改学生信息", items, 3);
    if (op < 1 || op == 3)
        return;
    if (op == 2)
    {
        char chosen[MAX_TEACHERS_PER_STUDENT][20];
        int n = selectTeachers(chosen);
        if (n <= 0 || !confirmAction("1确认修改 / 0取消: "))
            return;
        memset(students[s].teachers, 0, sizeof(students[s].teachers));
        memcpy(students[s].teachers, chosen, (size_t)n * 20);
        students[s].teacherCount = n;
        auditEvent("teacher-assignment", students[s].stuNum);
    }
    else
    {
        char name[20];
        if (readPersonName(name, 20, "学生姓名 (:q取消): ") != INPUT_OK || !confirmAction("1确认修改 / 0取消: "))
            return;
        strcpy(students[s].name, name);
        for (int i = 0; i < userCount; i++)
            if (users[i].type == STUDENT && !strcmp(users[i].username, students[s].stuNum))
                strcpy(userDisplayNames[i], name);
        auditEvent("student-name", students[s].stuNum);
    }
    if (saveAll())
        printf("学生信息更新成功。\n");
}
void editTeacher(void)
{
    if (currentUser.type != ADMIN)
        return;
    char id[20], name[60];
    if (readTeacherId(id, 20, "教师工号 (:q取消): ") != INPUT_OK)
        return;
    int u = -1;
    for (int i = 0; i < userCount; i++)
        if (users[i].type == TEACHER && !strcmp(users[i].username, id))
            u = i;
    if (u < 0)
    {
        printf("教师不存在。\n");
        return;
    }
    if (readPersonName(name, 60, "教师姓名 (:q取消): ") != INPUT_OK || !confirmAction("1确认修改 / 0取消: "))
        return;
    strcpy(userDisplayNames[u], name);
    auditEvent("teacher-name", id);
    if (saveAll())
        printf("教师姓名更新成功。\n");
}
void editRecords(void)
{
    editStudent();
}
void teacherList(void)
{
    const int w[] = {6, 20, 24, 24};
    const char *h[] = {"序号", "工号", "姓名", "科目"};
    tableStart(4, w, h);
    int n = 0;
    for (int i = 0; i < userCount; i++)
        if (users[i].type == TEACHER)
        {
            tableRow("dsss", ++n, users[i].username, displayName(users[i].username), users[i].teachCourse);
            if (n % 10 == 0)
            {
                tableEnd();
                if (!pageNext(n))
                    return;
                tableStart(4, w, h);
            }
        }
    tableEnd();
    printf("教师人数: %d\n", n);
}
void systemHome(void)
{
    int teachers = 0, scores = 0;
    for (int i = 0; i < userCount; i++)
        if (users[i].type == TEACHER)
            teachers++;
    for (int s = 0; s < studentCount; s++)
        scores += students[s].courseCount;
    printf("系统首页 学生: %d 教师: %d 班级: %d 成绩记录: %d\n", studentCount, teachers, classCount, scores);
}
void studentOverview(void)
{
    int s = pickStudent();
    if (s < 0)
        return;
    int c = studentClass(students[s].stuNum);
    printf("学生: %s 学号: %s 班级: %s\n", students[s].name, students[s].stuNum,
           c < 0 ? "未选择" : schoolClasses[c].name);
    for (int j = 0; j < students[s].teacherCount; j++)
        printf("教师: %s %s\n", students[s].teachers[j], displayName(students[s].teachers[j]));
    printStudentCourses(&students[s]);
}
void teacherOverview(void)
{
    char id[20];
    if (readTeacherId(id, 20, "教师工号 (:q取消): ") != INPUT_OK)
        return;
    int u = -1;
    for (int i = 0; i < userCount; i++)
        if (users[i].type == TEACHER && !strcmp(users[i].username, id))
            u = i;
    if (u < 0)
    {
        printf("教师不存在。\n");
        return;
    }
    printf("教师: %s %s 科目: %s\n", id, displayName(id), users[u].teachCourse);
    const int w[] = {20, 20, 24};
    const char *h[] = {"学号", "姓名", "班级"};
    tableStart(3, w, h);
    int n = 0;
    for (int s = 0; s < studentCount; s++)
        for (int j = 0; j < students[s].teacherCount; j++)
            if (!strcmp(students[s].teachers[j], id))
            {
                int c = studentClass(students[s].stuNum);
                tableRow("sss", students[s].stuNum, students[s].name, c < 0 ? "未选择" : schoolClasses[c].name);
                n++;
                if (n % 10 == 0)
                {
                    tableEnd();
                    if (!pageNext(n))
                        return;
                    tableStart(3, w, h);
                }
            }
    tableEnd();
    printf("授权学生人数: %d\n", n);
}
void classOverview(void)
{
    int c = selectClass();
    if (c < 0)
        return;
    printf("班级: %s 人数: %d 状态: %s\n", schoolClasses[c].name, schoolClasses[c].count,
           classInactive[c] ? "停用" : "启用");
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
    const char *m[] = {"全部", "按班级", "返回上一级"};
    int mode = menuChoice("列表范围", m, 3), c = -1;
    if (mode < 1 || mode == 3)
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
    if (safeReadString(key, sizeof(key), "学号/部分姓名 (:q取消): ") != INPUT_OK)
        return;
    const char *m[] = {"所有班级", "选择班级", "返回上一级"};
    int mode = menuChoice("搜索范围", m, 3), c = -1;
    if (mode < 1 || mode == 3)
        return;
    if (mode == 2)
    {
        c = selectClass();
        if (c < 0)
            return;
    }
    renderList(0, c, key);
}
