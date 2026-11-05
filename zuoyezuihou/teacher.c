#include "teacher.h"
#include "maintenance.h"
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <windows.h>

// 计算总分（静态函数，仅在本文件使用）


// 按特定科目查找成绩索引（静态函数，仅在本文件使用）
static int findCourseIndex(Student* student, const char* courseName) {
    for (int i = 0; i < student->courseCount; i++) {
        if (strcmp(student->courses[i].course, courseName) == 0) {
            return i;
        }
    }
    return -1;
}

// 检查教师是否有权限访问该学生（静态函数，仅在本文件使用）
static int teacherHasAccess(Student* student, const char* teacherName) {
    for (int i = 0; i < student->teacherCount; i++) {
        if (strcmp(student->teachers[i], teacherName) == 0) {
            return 1;  // 有权限
        }
    }
    return 0;  // 无权限
}

// 添加成绩
void addScores() {
    char stuNum[STU_NUM_LEN] = { 0 };

    printf("\n[添加成绩]\n");
    int selectedClass = selectClass();
    if (selectedClass < 0) return;
    if (classInactive[selectedClass]) { printf("班级已停用，不能添加新成绩。\n"); return; }
    printf("班级学生（你有授课权限）:\n");
    const int widths[] = {20, 16};
    const char* headers[] = {"学号", "姓名"};
    tableStart(2, widths, headers);
    for (int i = 0; i < studentCount; ++i)
        if (studentClass(students[i].stuNum) == selectedClass && teacherHasAccess(&students[i], currentUser.username))
            tableRow("ss", students[i].stuNum, students[i].name);
    tableEnd();
    safeReadString(stuNum, sizeof(stuNum), "输入学生学号: ");

    int studentIndex = findStudentIndex(stuNum);
    if (studentIndex == -1) {
        printf("该学生不存在，请先在管理员端添加学生用户\n");
        return;
    }

    if (studentClass(stuNum) != selectedClass) {
        printf("该学生不属于所选班级！\n"); return;
    }
    // 检查当前教师是否有权限添加该学生的成绩
    if (!teacherHasAccess(&students[studentIndex], currentUser.username)) {
        printf("错误：你不是该学生的授课教师，无权添加成绩！\n");
        return;
    }

    Student* student = &students[studentIndex];
    if (findCourseIndex(student, currentUser.teachCourse) >= 0) {
        printf("该课程已有成绩，请使用修改成绩功能！\n"); return;
    }
    if (student->courseCount >= MAX_COURSES) {
        printf("课程数量已达上限！\n"); return;
    }
    float score = safeReadFloat("输入课程成绩 (0-100): ", 0, 100);
    int index = student->courseCount++;
    strcpy(student->courses[index].course, currentUser.teachCourse);
    student->courses[index].score = score;
    if (saveAll()) printf("成绩添加完成！\n");
}

// 修改成绩
void modifyScores() {
    printf("\n[修改成绩]\n");

    // 收集当前教师有权限修改成绩的学生列表
    int teacherStudents[MAX_STUDENTS];
    int teacherStudentCount = 0;

    for (int i = 0; i < studentCount; i++) {
        if (teacherHasAccess(&students[i], currentUser.username)) {
            teacherStudents[teacherStudentCount++] = i;
        }
    }

    if (teacherStudentCount == 0) {
        printf("你没有可修改成绩的学生！\n");
        return;
    }

    // 显示学生列表
    printf("你所教的学生列表:\n");
    const int widths[] = {6, 20, 16};
    const char* headers[] = {"序号", "学号", "姓名"};
    tableStart(3, widths, headers);
    for (int i = 0; i < teacherStudentCount; ++i) {
        int index = teacherStudents[i];
        tableRow("dss", i+1, students[index].stuNum, students[index].name);
    }
    tableEnd();

    // 选择学生
    int selection = safeReadInt("选择学生序号: ", 1, teacherStudentCount) - 1;
    int studentIndex = teacherStudents[selection];

    if (students[studentIndex].courseCount == 0) {
        printf("该学生暂无成绩记录！\n"); return;
    }
    // 显示学生已有成绩
    printf("\n学生 %s (学号: %s) 的成绩:\n",
        students[studentIndex].name,
        students[studentIndex].stuNum);
    printStudentCourses(&students[studentIndex]);

    // 检查是否有成绩可修改
    if (students[studentIndex].courseCount == 0) {
        printf("该学生暂无成绩记录！\n");
        return;
    }

    // 选择要修改的课程
    int courseIndex = safeReadInt("选择要修改的课程序号: ", 1, students[studentIndex].courseCount) - 1;

    // 检查课程是否属于当前教师所授科目
    if (strcmp(students[studentIndex].courses[courseIndex].course, currentUser.teachCourse) != 0) {
        printf("错误：该课程不是你所教授的科目！\n");
        return;
    }

    // 修改成绩
    float newScore = safeReadFloat("输入新成绩: ", 0.0f, 100.0f);
    students[studentIndex].courses[courseIndex].score = newScore;

    if (saveAll()) printf("成绩修改成功！\n");
}

// 查询成绩
void queryScores(void) { classQuery(); }

// 计算平均分
void calculateAverages(void) { classStatistics(); }

// 成绩排名 - 修改后的函数
void rankStudents(void) { classRanking(); }

// 显示及格率
void showPassRate(void) { classStatistics(); }

// CSV validation, preview and atomic import live in csv.c.
void importScores(void) { csvImport(); }

// 教师菜单
void teacherMenu(void) {
    int choice;

    while (1) {
        clearScreen();
        printf("\n[教师菜单] - 当前用户: %s (教授科目: %s)\n",
            displayName(currentUser.username), currentUser.teachCourse);
        printf("1. 添加成绩\n2. 修改成绩\n3. 按班级查询（授权学生完整成绩）\n");
        printf("4. 计算平均分\n5. 成绩排名\n6. 及格率统计\n");
        printf("7. 返回\n8. 文件导入成绩\n9. 查看所有学生\n10. 查看所有成绩\n11. 导出当前科目CSV\n12. 搜索学生\n选择: ");

        choice = safeReadInt("", 1, 12);

        switch (choice) {
        case 1:
            addScores();
            break;
        case 2:
            modifyScores();
            break;
        case 3:
            queryScores();
            break;
        case 4:
            calculateAverages();
            break;
        case 5:
            rankStudents();
            break;
        case 6:
            showPassRate();
            break;
        case 7:
            return;
        case 8:
            importScores(); break;
        case 9: listStudentsAndScores(0); break;
        case 10: listStudentsAndScores(1); break;
        case 11: csvExport(); break;
        case 12: searchRecords(); break;
        }
        pauseScreen();
    }
}