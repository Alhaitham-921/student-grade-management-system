#include "system.h"
#include <windows.h>
#include "admin.h"
#include "teacher.h"
#include "student.h"

int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    loadAll();
    initSystem();
    saveAll();

    int choice;

    while (1) {
        clearScreen();
        printf("\n[学生成绩管理系统]\n");
        printf("1. 管理员登录\n2. 教师登录\n3. 学生登录\n4. 退出\n选择: ");

        choice = safeReadInt("", 1, 4);

        switch (choice) {
        case 1:
            if (login(ADMIN)) {
                adminMenu();  // 调用管理员菜单
            }
            break;
        case 2:
            if (login(TEACHER)) {
                teacherMenu();  // 调用教师菜单
            }
            break;
        case 3:
            if (login(STUDENT)) {
                studentMenu();  // 调用学生菜单
            }
            break;
        case 4:
            printf("感谢使用系统，再见！\n");
            return 0;
        }
    }

    return 0;
}