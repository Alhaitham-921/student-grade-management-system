#include "maintenance.h"

int studentClass(const char *id)
{
    for (int i = 0; i < classCount; i++)
        for (int j = 0; j < schoolClasses[i].count; j++)
            if (!strcmp(id, schoolClasses[i].members[j]))
                return i;
    return -1;
}
void removeClassMember(const char *id)
{
    for (int i = 0; i < classCount; i++)
        for (int j = 0; j < schoolClasses[i].count; j++)
            if (!strcmp(id, schoolClasses[i].members[j]))
            {
                int n = schoolClasses[i].count - j - 1;
                if (n > 0)
                    memmove(schoolClasses[i].members[j], schoolClasses[i].members[j + 1], (size_t)n * STU_NUM_LEN);
                schoolClasses[i].count--;
                break;
            }
}
void listClasses(void)
{
    const int w[] = {6, 40, 10, 8};
    const char *h[] = {"序号", "班级", "人数", "状态"};
    tableStart(4, w, h);
    for (int i = 0; i < classCount; i++)
        tableRow("dsds", i + 1, schoolClasses[i].name, schoolClasses[i].count, classInactive[i] ? "停用" : "启用");
    tableEnd();
    if (!classCount)
        printf("暂无班级，请联系管理员添加。\n");
}
int selectClass(void)
{
    listClasses();
    if (!classCount)
        return -1;
    printf("%d. 返回上一级\n", classCount + 1);
    int choice = safeReadInt("选择班级序号: ", 1, classCount + 1);
    return choice < 1 || choice == classCount + 1 ? -1 : choice - 1;
}
static int readName(char *name, size_t size)
{
    for (;;)
    {
        if (safeReadString(name, size, "班级名称 (:q取消): ") != INPUT_OK)
            return 0;
        size_t n = strlen(name);
        if (name[0] == ' ' || name[n - 1] == ' ')
        {
            printf("班级名称首尾不能有空格。\n");
            continue;
        }
        return 1;
    }
}
void addClass(void)
{
    if (currentUser.type != ADMIN)
    {
        printf("无权管理班级。\n");
        return;
    }
    if (classCount >= MAX_CLASSES)
    {
        printf("班级数量已达上限！\n");
        return;
    }
    char name[CLASS_NAME_LEN];
    if (!readName(name, sizeof(name)))
        return;
    for (int i = 0; i < classCount; i++)
        if (!strcmp(name, schoolClasses[i].name))
        {
            printf("班级已存在！\n");
            return;
        }
    memset(&schoolClasses[classCount], 0, sizeof(SchoolClass));
    strcpy(schoolClasses[classCount].name, name);
    classInactive[classCount++] = 0;
    auditEvent("class-create", name);
    if (saveAll())
        printf("班级添加成功！\n");
}
void chooseMyClass(void)
{
    printf("班级由管理员统一分配，学生无权自行换班。\n");
}
OperationResult assignClassMember(const char *id, int c)
{
    if (currentUser.type != ADMIN)
        return RESULT_DENIED;
    if (c < 0 || c >= classCount || classInactive[c] || findStudentIndex(id) < 0)
        return RESULT_INVALID;
    if (studentClass(id) == c)
        return RESULT_OK;
    if (schoolClasses[c].count >= MAX_STUDENTS)
        return RESULT_INVALID;
    removeClassMember(id);
    strcpy(schoolClasses[c].members[schoolClasses[c].count++], id);
    return saveAll() ? RESULT_OK : RESULT_IO;
}
static void assignClass(void)
{
    char id[20];
    if (readStudentId(id, sizeof(id), "学号 (:q取消): ") != INPUT_OK)
        return;
    int s = findStudentIndex(id);
    if (s < 0)
    {
        printf("学生不存在。\n");
        return;
    }
    int c = selectClass();
    if (c < 0)
        return;
    if (classInactive[c])
    {
        printf("班级已停用，不能分班。\n");
        return;
    }
    if (studentClass(id) == c)
    {
        printf("学生已在该班级。\n");
        return;
    }
    if (schoolClasses[c].count >= MAX_STUDENTS)
    {
        printf("班级已满！\n");
        return;
    }
    if (assignClassMember(id, c) == RESULT_OK)
        printf("分班成功；已保留原成绩。\n");
}
static void members(void)
{
    int c = selectClass();
    if (c < 0)
        return;
    const int w[] = {6, 20, 20};
    const char *h[] = {"序号", "学号", "姓名"};
    int count = 0;
    tableStart(3, w, h);
    for (int i = 0; i < studentCount; i++)
        if (studentClass(students[i].stuNum) == c)
        {
            tableRow("dss", ++count, students[i].stuNum, students[i].name);
            if (count % 10 == 0)
            {
                tableEnd();
                if (!pageNext(count))
                    return;
                tableStart(3, w, h);
            }
        }
    tableEnd();
    printf("班级人数: %d\n", schoolClasses[c].count);
}
static void renameClass(void)
{
    int c = selectClass();
    if (c < 0)
        return;
    char name[CLASS_NAME_LEN];
    if (!readName(name, sizeof(name)))
        return;
    for (int i = 0; i < classCount; i++)
        if (i != c && !strcmp(name, schoolClasses[i].name))
        {
            printf("班级已存在！\n");
            return;
        }
    char detail[160];
    snprintf(detail, sizeof(detail), "before=%s after=%s", schoolClasses[c].name, name);
    strcpy(schoolClasses[c].name, name);
    auditEvent("class-rename", detail);
    if (saveAll())
        printf("班级更新成功。\n");
}
static void toggleClass(void)
{
    int c = selectClass();
    if (c < 0)
        return;
    classInactive[c] = !classInactive[c];
    auditEvent(classInactive[c] ? "class-disable" : "class-enable", schoolClasses[c].name);
    if (saveAll())
        printf("班级更新成功。\n");
}
void classManagement(void)
{
    if (currentUser.type != ADMIN)
        return;
    const MenuItem m[] = {{"创建班级", addClass},  {"查看班级列表", listClasses}, {"查看成员", members},
                          {"重命名", renameClass}, {"分班/转班", assignClass},    {"停用/启用", toggleClass},
                          {"返回上一级", NULL}};
    runMenu("管理员 / 基础信息管理 / 班级管理", m, 7);
}
