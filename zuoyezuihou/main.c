#include "maintenance.h"
#include <windows.h>
#include "admin.h"
#include "teacher.h"
#include "student.h"
int main(void)
{
#ifdef SGMS_TESTING
    if(getenv("SGMS_CONSOLE_PASSWORD_TEST")){extern int verifyConsolePassword(void);return verifyConsolePassword();}
#endif
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    if (!storageOpen())
        return EXIT_FAILURE;
    loadAll();
    initSystem();
    if (!saveAll())
        return EXIT_FAILURE;
    if (!migrateTeachers())
        return EXIT_FAILURE;
#ifdef SGMS_INITIALIZER
    strcpy(currentUser.username, "maintenance");
    currentUser.type = ADMIN;
    return seedDemoData(1) ? 0 : 1;
#endif
    while (!inputFinished())
    {
        clearScreen();
        const char *m[] = {"管理员登录", "教师登录", "学生登录", "退出系统"};
        int choice = menuChoice("学生成绩管理系统", m, 4);
        if (choice < 1 || choice == 4)
            break;
        UserType type = choice == 1 ? ADMIN : choice == 2 ? TEACHER : STUDENT;
        if (login(type))
        {
            if (type == ADMIN)
                adminMenu();
            else if (type == TEACHER)
                teacherMenu();
            else
                studentMenu();
        }
        memset(&currentUser, 0, sizeof(currentUser));
    }
    printf("输入结束或已退出，系统正常结束。\n");
    return 0;
}
