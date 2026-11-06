#include "student.h"
#include "maintenance.h"
#include "admin.h" // 用于访问 changePassword 函数

// 查看我的成绩
void viewMyScores(void)
{
    Student *student = findStudentByUsername(currentUser.username);
    if (!student)
    {
        printf("未找到你的学生记录！\n");
        return;
    }

    // 显示学生信息
    printf("\n学生: %s (学号: %s)\n", student->name, student->stuNum);
    int c = studentClass(student->stuNum);
    printf("班级: %s\n", c < 0 ? "未选择" : schoolClasses[c].name);
    printf("授课教师: ");
    if (student->teacherCount == 0)
        printf("暂无授课教师\n");
    for (int i = 0; i < student->teacherCount; i++)
    {
        printf("%s%s", displayName(student->teachers[i]), i < student->teacherCount - 1 ? ", " : "\n");
    }

    // 显示学生成绩
    printf("\n你的成绩:\n");
    printStudentCourses(student);
    if (student->courseCount)
        printf("平均分: %.1f\n", student->average);
    else
        printf("平均分: --（尚无已录入成绩，不代表零分）\n");
}

static void myClass(void)
{
    int c = studentClass(currentUser.username);
    printf("班级: %s（管理员分配）\n", c < 0 ? "未选择" : schoolClasses[c].name);
}
void studentMenu(void)
{
    if (currentUser.type != STUDENT)
        return;
    const MenuItem m[] = {{"查看我的成绩", viewMyScores},
                          {"修改密码", changePassword},
                          {"查看班级（管理员分配）", myClass},
                          {"退出登录", NULL}};
    runMenu("学生菜单", m, 4);
}
