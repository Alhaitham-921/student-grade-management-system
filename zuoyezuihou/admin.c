#include "admin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 检查学号是否已存在
static int studentIdExists(const char* stuNum) {
    for (int i = 0; i < studentCount; i++) {
        if (strcmp(students[i].stuNum, stuNum) == 0) {
            return 1; // 学号已存在
        }
    }
    return 0; // 学号不存在
}

// 添加学生
void addStudent() {
    if (studentCount >= MAX_STUDENTS || userCount >= MAX_USERS) {
        printf("学生或用户数量已达上限！\n"); return;
    }
    Student newStudent = { 0 };
    newStudent.teacherCount = 0;  // 初始化教师数量

    printf("\n[添加学生]\n");

    // 读取并验证学号唯一性
    while (1) {
        safeReadString(newStudent.stuNum, sizeof(newStudent.stuNum), "学号: ");

        if (studentIdExists(newStudent.stuNum) || userExists(newStudent.stuNum)) {
            printf("错误：该学号已存在！请重新输入。\n");
        }
        else {
            break; // 学号唯一，继续添加
        }
    }

    safeReadString(newStudent.name, sizeof(newStudent.name), "姓名: ");

    // 显示可选教师列表
    printf("\n可选教师列表:\n");
    const int widths[] = {6, 20, 24};
    const char* headers[] = {"序号", "教师", "任教科目"};
    tableStart(3, widths, headers);
    int availableTeachers[MAX_USERS];
    int availableTeacherCount = 0;

    for (int i = 0; i < userCount; i++) {
        if (users[i].type == TEACHER) {
            availableTeachers[availableTeacherCount++] = i;
            tableRow("dss",
                availableTeacherCount, users[i].username, users[i].teachCourse);
        }
    }

    tableEnd();
    if (availableTeacherCount == 0) {
        printf("暂无可用教师！\n");
        return;
    }

    // 为学生分配教师
    printf("\n请为学生分配教师（最多 %d 位，输入0结束）:\n", MAX_TEACHERS_PER_STUDENT);

    while (1) {
        if (newStudent.teacherCount >= MAX_TEACHERS_PER_STUDENT) {
            printf("已达到教师数量上限！\n");
            break;
        }

        int choice = safeReadInt("选择教师序号 (0结束): ", 0, availableTeacherCount);

        if (choice == 0) break; // 结束选择

        int teacherIndex = availableTeachers[choice - 1];

        // 检查是否已选择该教师
        int alreadySelected = 0;
        for (int i = 0; i < newStudent.teacherCount; i++) {
            if (strcmp(users[teacherIndex].username, newStudent.teachers[i]) == 0) {
                alreadySelected = 1;
                break;
            }
        }

        if (alreadySelected) {
            printf("该教师已选择！\n");
        }
        else {
            // 添加教师到学生记录
            strcpy(newStudent.teachers[newStudent.teacherCount++], users[teacherIndex].username);
            printf("已选择教师: %s (科目: %s)\n",
                users[teacherIndex].username, users[teacherIndex].teachCourse);
        }
    }

    if (newStudent.teacherCount == 0) {
        printf("至少需要选择一位教师！\n");
        return;
    }

    // 初始化其他学生信息
    newStudent.courseCount = 0;
    newStudent.average = 0;

    // 添加学生到系统
    if (studentCount < MAX_STUDENTS) {
        students[studentCount++] = newStudent;
        printf("学生 %s (学号: %s) 添加成功！\n", newStudent.name, newStudent.stuNum);

        // 为学生创建默认用户账号（用户名=姓名，密码=学号）
        User studentUser = { 0 };
        strcpy(studentUser.username, newStudent.stuNum);
        hashPassword(newStudent.stuNum, studentUser.password);
        studentUser.type = STUDENT;

        if (userCount < MAX_USERS) {
            users[userCount++] = studentUser;
        }
        else {
            printf("警告：用户数量已达上限，未创建学生账号！\n");
        }

        saveAll(); // 保存所有数据
    }
    else {
        printf("错误：学生数量已达上限！\n");
    }
}

// 添加教师
void addTeacher() {
    User newTeacher = { 0 };
    newTeacher.type = TEACHER;

    printf("\n[添加教师]\n");

    // 读取并验证用户名唯一性
    while (1) {
        safeReadString(newTeacher.username, sizeof(newTeacher.username), "教师姓名: ");

        if (userExists(newTeacher.username)) {
            printf("错误：该用户名已存在！请重新输入。\n");
        }
        else {
            break; // 用户名唯一，继续添加
        }
    }

    char initialPassword[20];
    safeReadString(initialPassword, sizeof(initialPassword), "初始密码: ");
    hashPassword(initialPassword, newTeacher.password);
    safeReadString(newTeacher.teachCourse, sizeof(newTeacher.teachCourse), "授课科目: ");

    // 添加教师到系统
    if (userCount < MAX_USERS) {
        users[userCount++] = newTeacher;
        printf("教师 %s (科目: %s) 添加成功！\n", newTeacher.username, newTeacher.teachCourse);
        saveAll(); // 保存所有数据
    }
    else {
        printf("错误：用户数量已达上限！\n");
    }
}

// 删除学生
void deleteStudent() {
    char stuNum[STU_NUM_LEN] = { 0 };
    printf("\n[删除学生]\n");
    safeReadString(stuNum, sizeof(stuNum), "请输入要删除的学生学号: ");

    int index = findStudentIndex(stuNum);
    if (index == -1) {
        printf("错误：未找到该学生！\n");
        return;
    }

    // 确认删除
    char confirm[10] = { 0 };
    printf("确定要删除学生 %s (学号: %s) 吗？(yes/no): ",
        students[index].name, students[index].stuNum);
    safeReadString(confirm, sizeof(confirm), "");

    if (strcmp(confirm, "yes") != 0) {
        printf("操作已取消！\n");
        return;
    }

    char deletedId[STU_NUM_LEN];
    strcpy(deletedId, students[index].stuNum);
    removeClassMember(deletedId);
    // 从数组中移除学生
    for (int i = index; i < studentCount - 1; i++) {
        students[i] = students[i + 1];
    }
    studentCount--;

    // 同时删除对应的用户账号
    for (int i = 0; i < userCount; i++) {
        if (users[i].type == STUDENT && strcmp(users[i].username, deletedId) == 0) {
            for (int j = i; j < userCount - 1; j++) {
                users[j] = users[j + 1];
            }
            userCount--;
            break;
        }
    }

    printf("学生删除成功！\n");
    saveAll(); // 保存所有数据
}

// 查看所有用户
void viewAllUsers() {
    printf("\n[所有用户]\n");
    const int widths[] = {10, 20, 24, 10};
    const char* headers[] = {"类型", "用户名", "任教科目", "密码"};
    tableStart(4, widths, headers);
    for (int i = 0; i < userCount; ++i) {
        const char* type = users[i].type == ADMIN ? "管理员" : users[i].type == TEACHER ? "教师" : "学生";
        tableRow("ssss", type, users[i].username, users[i].type == TEACHER ? users[i].teachCourse : "--", "******");
    }
    tableEnd();
}

// 修改密码
void changePassword() {
    char oldPass[20] = { 0 };
    char newPass[20] = { 0 };
    char confirmPass[20] = { 0 };

    printf("\n[修改密码]\n");
    safeReadString(oldPass, sizeof(oldPass), "请输入当前密码: ");

    // 验证当前密码
    if (!verifyPassword(oldPass, currentUser.password)) {
        printf("错误：当前密码不正确！\n");
        return;
    }

    // 获取并确认新密码
    while (1) {
        safeReadString(newPass, sizeof(newPass), "请输入新密码: ");
        safeReadString(confirmPass, sizeof(confirmPass), "请再次输入新密码: ");

        if (strlen(newPass) < 6) { printf("新密码至少需要6个字节！\n"); continue; }
        if (strcmp(newPass, confirmPass) != 0) {
            printf("两次输入的密码不一致！请重新输入。\n");
        }
        else {
            break;
        }
    }

    // 更新密码
    hashPassword(newPass, currentUser.password);

    // 在用户数组中更新密码
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].username, currentUser.username) == 0) {
            strcpy(users[i].password, currentUser.password);
            break;
        }
    }

    printf("密码修改成功！\n");
    saveAll(); // 保存所有数据
}

// 清空所有用户
void clearAllUsers() {
    char confirm[20] = { 0 };
    printf("\n警告：此操作将删除所有用户数据（学生、教师），仅保留默认管理员账号！\n");
    safeReadString(confirm, sizeof(confirm), "确认执行此操作？(输入 'confirm' 确认): ");

    if (strcmp(confirm, "confirm") != 0) {
        printf("操作已取消！\n");
        return;
    }

    // 清空用户数组，保留默认管理员
    userCount = 1;
    strcpy(users[0].username, "admin");
    strcpy(users[0].password, currentUser.password);
    users[0].type = ADMIN;

    // 清空学生数组
    studentCount = 0;
    for (int i = 0; i < classCount; ++i) schoolClasses[i].count = 0;

    printf("所有用户数据已清空！\n");
    saveAll(); // 保存所有数据
}

// 管理员菜单
void adminMenu(void) {
    int choice;

    while (1) {
        clearScreen();
        printf("\n[管理员菜单] - 当前用户: %s\n", currentUser.username);
        printf("1. 添加学生\n2. 添加教师\n3. 删除学生\n");
        printf("4. 查看所有用户\n5. 修改密码\n6. 清空所有用户\n");
        printf("7. 返回\n8. 添加班级\n9. 查看班级\n10. 查看所有学生\n11. 查看所有成绩\n选择: ");

        choice = safeReadInt("", 1, 11);

        switch (choice) {
        case 1: addStudent(); break;
        case 2: addTeacher(); break;
        case 3: deleteStudent(); break;
        case 4: viewAllUsers(); break;
        case 5: changePassword(); break;
        case 6: clearAllUsers(); break;
        case 7: return;
        case 8: addClass(); break;
        case 9: listClasses(); break;
        case 10: listStudentsAndScores(0); break;
        case 11: listStudentsAndScores(1); break;
        }

        pauseScreen();
    }
}