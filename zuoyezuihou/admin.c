#include "admin.h"
#include "maintenance.h"

void addTeacher(void)
{
    if (currentUser.type != ADMIN || userCount >= MAX_USERS)
    {
        printf("无权限或用户数量已达上限。\n");
        return;
    }
    User teacher = {0};
    char name[60], password[20];
    teacher.type = TEACHER;
    for (;;)
    {
        if (readTeacherId(teacher.username, 20, "教师工号 (:q取消): ") != INPUT_OK)
            return;
        if (!userExists(teacher.username))
            break;
        printf("该工号已存在，请重新输入。\n");
    }
    if (readPersonName(name, sizeof(name), "教师姓名 (:q取消): ") != INPUT_OK)
        return;
    if (safeReadString(teacher.teachCourse, 30, "任教科目 (:q取消): ") != INPUT_OK)
        return;
    for (;;)
    {
        if (readPassword(password, sizeof(password), "初始密码（至少6字节，:q取消）: ") != INPUT_OK)
            return;
        if (strlen(password) >= 6)
            break;
        printf("密码至少6字节。\n");
    }
    printf("教师预览 工号: %s 姓名: %s 科目: %s\n", teacher.username, name, teacher.teachCourse);
    if (!confirmAction("1确认添加 / 0取消: "))
        return;
    hashPassword(password, teacher.password);
    if (createTeacher(&teacher, name) == RESULT_OK)
        printf("教师添加成功！\n");
}
void addStudent(void)
{
    if (currentUser.type != ADMIN || studentCount >= MAX_STUDENTS || userCount >= MAX_USERS)
    {
        printf("无权限或学生/用户数量已达上限。\n");
        return;
    }
    Student st = {0};
    for (;;)
    {
        if (readStudentId(st.stuNum, 20, "学生学号 (:q取消): ") != INPUT_OK)
            return;
        if (!userExists(st.stuNum) && findStudentIndex(st.stuNum) < 0)
            break;
        printf("该学号已存在，请重新输入。\n");
    }
    if (readPersonName(st.name, 20, "学生姓名 (:q取消): ") != INPUT_OK)
        return;
    int c = selectClass();
    if (c < 0)
        return;
    if (classInactive[c])
    {
        printf("班级已停用，不能分班。\n");
        return;
    }
    st.teacherCount = selectTeachers(st.teachers);
    if (st.teacherCount <= 0)
        return;
    printf("学生预览 学号: %s 姓名: %s 班级: %s 教师数: %d\n", st.stuNum, st.name, schoolClasses[c].name,
           st.teacherCount);
    if (!confirmAction("1确认添加 / 0取消: "))
        return;
    if (createStudent(&st, c) == RESULT_OK)
        printf("学生添加成功！\n");
}
void deleteStudent(void)
{
    if (currentUser.type != ADMIN)
        return;
    char id[20], confirmation[20];
    if (readStudentId(id, 20, "删除学生学号 (:q取消): ") != INPUT_OK)
        return;
    int s = findStudentIndex(id);
    if (s < 0)
    {
        printf("学生不存在。\n");
        return;
    }
    printf("将删除 %s %s 及关联账号和班级归属。\n", id, students[s].name);
    if (safeReadString(confirmation, 20, "输入 yes 确认，其他文字取消: ") != INPUT_OK || strcmp(confirmation, "yes"))
        return;
    removeClassMember(id);
    for (int i = s; i < studentCount - 1; i++)
        students[i] = students[i + 1];
    studentCount--;
    for (int u = 0; u < userCount; u++)
        if (users[u].type == STUDENT && !strcmp(users[u].username, id))
        {
            for (int j = u; j < userCount - 1; j++)
            {
                users[j] = users[j + 1];
                strcpy(userDisplayNames[j], userDisplayNames[j + 1]);
            }
            userCount--;
            break;
        }
    auditEvent("student-delete", id);
    if (saveAll())
        printf("学生删除成功！\n");
}
void viewAllUsers(void)
{
    const int w[] = {10, 20, 20, 24, 10};
    const char *h[] = {"类型", "账号/主码", "姓名", "科目", "密码"};
    tableStart(5, w, h);
    for (int i = 0; i < userCount; i++)
    {
        tableRow("sssss",
                 users[i].type == ADMIN     ? "管理员"
                 : users[i].type == TEACHER ? "教师"
                                            : "学生",
                 users[i].username, displayName(users[i].username),
                 users[i].type == TEACHER ? users[i].teachCourse : "--", "******");
        if ((i + 1) % 10 == 0 && i + 1 < userCount)
        {
            tableEnd();
            if (!pageNext(i + 1))
                return;
            tableStart(5, w, h);
        }
    }
    tableEnd();
}
void changePassword(void)
{
    char old[20], pass[20], again[20];
    if (readPassword(old, 20, "当前密码 (:q取消): ") != INPUT_OK)
        return;
    if (!verifyPassword(old, currentUser.password))
    {
        printf("当前密码不正确。\n");
        return;
    }
    for (;;)
    {
        if (readPassword(pass, 20, "新密码 (:q取消): ") != INPUT_OK ||
            readPassword(again, 20, "再次输入新密码 (:q取消): ") != INPUT_OK)
            return;
        if (strlen(pass) < 6)
        {
            printf("新密码至少6个字节。\n");
            continue;
        }
        if (strcmp(pass, again))
        {
            printf("两次输入不一致。\n");
            continue;
        }
        break;
    }
    for (int i = 0; i < userCount; i++)
        if (!strcmp(users[i].username, currentUser.username))
        {
            hashPassword(pass, users[i].password);
            auditEvent("password-change", currentUser.username);
            if (saveAll())
            {
                currentUser = users[i];
                printf("密码修改成功！\n");
            }
            return;
        }
}
void clearAllUsers(void)
{
    if (currentUser.type != ADMIN)
        return;
    char text[20];
    printf("危险操作：删除全部学生和教师，仅保留当前管理员及班级定义。\n");
    if (safeReadString(text, 20, "输入 confirm 确认 (:q取消): ") != INPUT_OK || strcmp(text, "confirm"))
        return;
    users[0] = currentUser;
    userCount = 1;
    studentCount = 0;
    demoSeedVersion = 0;
    memset(userDisplayNames, 0, sizeof(userDisplayNames));
    for (int i = 0; i < classCount; i++)
        schoolClasses[i].count = 0;
    auditEvent("users-clear", "all students and teachers");
    if (saveAll())
        printf("所有用户数据已清空！\n");
}
static void listStudents(void)
{
    filteredList(0);
}
static void studentManagement(void)
{
    const MenuItem m[] = {{"添加学生", addStudent},    {"查看学生列表", listStudents}, {"修改学生信息", editStudent},
                          {"删除学生", deleteStudent}, {"搜索学生", searchRecords},    {"返回上一级", NULL}};
    runMenu("管理员 / 基础信息管理 / 学生管理", m, 6);
}
static void teacherManagement(void)
{
    const MenuItem m[] = {{"添加教师", addTeacher},
                          {"查看教师列表", teacherList},
                          {"修改教师姓名", editTeacher},
                          {"维护学生授课关系", editTeaching},
                          {"返回上一级", NULL}};
    runMenu("管理员 / 基础信息管理 / 教师管理", m, 5);
}
static void basicManagement(void)
{
    const MenuItem m[] = {{"学生管理", studentManagement},
                          {"教师管理", teacherManagement},
                          {"班级管理", classManagement},
                          {"返回上一级", NULL}};
    runMenu("管理员 / 基础信息管理", m, 4);
}
static void gradeManagement(void)
{
    const MenuItem m[] = {{"按班级/科目查看成绩", adminScoreQuery},
                          {"班级成绩统计", classStatistics},
                          {"导出CSV", csvExport},
                          {"返回上一级", NULL}};
    runMenu("管理员 / 成绩管理", m, 4);
}
static void queries(void)
{
    const MenuItem m[] = {{"学生综合信息", studentOverview},
                          {"教师授课信息", teacherOverview},
                          {"班级概况", classOverview},
                          {"返回上一级", NULL}};
    runMenu("管理员 / 综合查询", m, 4);
}
static void dangerous(void)
{
    const MenuItem m[] = {{"清空用户与学生数据", clearAllUsers}, {"返回上一级", NULL}};
    runMenu("管理员 / 系统管理 / 危险操作", m, 2);
}
static void settings(void)
{
    const MenuItem m[] = {{"修改本人密码", changePassword}, {"查看用户账号", viewAllUsers}, {"查看操作日志", showAudit},
                          {"补充示例数据", seedDemoMenu},   {"危险操作", dangerous},        {"返回上一级", NULL}};
    runMenu("管理员 / 系统管理", m, 6);
}
void adminMenu(void)
{
    if (currentUser.type != ADMIN)
        return;
    const MenuItem m[] = {{"系统首页", systemHome},      {"基础信息管理", basicManagement},
                          {"成绩管理", gradeManagement}, {"综合查询", queries},
                          {"系统管理", settings},        {"退出登录", NULL}};
    runMenu("学生成绩管理系统 / 管理员菜单", m, 6);
}
