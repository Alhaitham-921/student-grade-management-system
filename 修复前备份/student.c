#include "student.h"
#include "admin.h"  // 用于访问 changePassword 函数

// 查看我的成绩
void viewMyScores() {
    Student* student = findStudentByUsername(currentUser.username);
    if (!student) {
        printf("未找到你的学生记录！\n");
        return;
    }

    // 显示学生信息
    printf("\n学生: %s (学号: %s)\n", student->name, student->stuNum);
    printf("授课教师: ");
    for (int i = 0; i < student->teacherCount; i++) {
        printf("%s%s", student->teachers[i], i < student->teacherCount - 1 ? ", " : "\n");
    }

    // 显示学生成绩
    printf("\n你的成绩:\n");
    printf("课程                成绩\n");
    printf("-------------------------\n");

    for (int i = 0; i < student->courseCount; i++) {
        printf("%-20s %.1f\n",
            student->courses[i].course,
            student->courses[i].score);
    }

    printf("-------------------------\n");
    printf("平均分: %.1f\n", student->average);
}

// 学生菜单
void studentMenu(void) {
    int choice;

    while (1) {
        system("cls");
        printf("\n[学生菜单] - 当前用户: %s\n", currentUser.username);
        printf("1. 查看我的成绩\n2. 修改密码\n3. 返回\n选择: ");

        choice = safeReadInt("", 1, 3);

        switch (choice) {
        case 1:
            viewMyScores();
            break;
        case 2:
            changePassword();  // 复用管理员的修改密码函数
            break;
        case 3:
            return;
        }

        system("pause");
    }
}