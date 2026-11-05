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
    return safeReadInt("选择班级序号 (0取消): ", 0, classCount) - 1;
}
static void readName(char *name, size_t size)
{
    for (;;)
    {
        safeReadString(name, size, "班级名称: ");
        size_t n = strlen(name);
        if (name[0] == ' ' || name[n - 1] == ' ')
        {
            printf("班级名称首尾不能有空格。\n");
            continue;
        }
        break;
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
    readName(name, sizeof(name));
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
    safeReadString(id, sizeof(id), "学号 (0取消): ");
    if (!strcmp(id, "0"))
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
void classManagement(void)
{
    if (currentUser.type != ADMIN)
    {
        printf("无权管理班级。\n");
        return;
    }
    for (;;)
    {
        printf("\n[班级管理]\n1. 创建\n2. 重命名\n3. 查看成员\n4. 分班/转班\n5. 停用/启用\n6. 列表\n0. 返回\n");
        int op = safeReadInt("选择: ", 0, 6);
        if (!op)
            return;
        if (op == 1)
            addClass();
        if (op == 3)
            members();
        if (op == 4)
            assignClass();
        if (op == 6)
            listClasses();
        if (op == 2 || op == 5)
        {
            int c = selectClass();
            if (c < 0)
                continue;
            if (op == 2)
            {
                char name[CLASS_NAME_LEN];
                readName(name, sizeof(name));
                int duplicate = 0;
                for (int i = 0; i < classCount; i++)
                    if (i != c && !strcmp(name, schoolClasses[i].name))
                        duplicate = 1;
                if (duplicate)
                {
                    printf("班级已存在！\n");
                    continue;
                }
                char detail[160];
                snprintf(detail, sizeof(detail), "before=%s after=%s", schoolClasses[c].name, name);
                strcpy(schoolClasses[c].name, name);
                auditEvent("class-rename", detail);
            }
            else
            {
                classInactive[c] = !classInactive[c];
                auditEvent(classInactive[c] ? "class-disable" : "class-enable", schoolClasses[c].name);
            }
            if (saveAll())
                printf("班级更新成功。\n");
        }
    }
}
