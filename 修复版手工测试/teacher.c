#include "teacher.h"
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <windows.h>

// 计算总分（静态函数，仅在本文件使用）
static float calculateTotalScore(Student* student) {
    float total = 0;
    for (int i = 0; i < student->courseCount; i++) {
        total += student->courses[i].score;
    }
    return total;
}

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
    printf("成绩添加完成！\n");
    saveAll();
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

    printf("成绩修改成功！\n");
    saveAll();
}

// 查询成绩
void queryScores() {
    printf("\n[查询成绩]\n");
    printf("1. 按学生查询\n2. 按课程查询\n选择: ");
    int choice = safeReadInt("", 1, 2);

    if (choice == 1) {
        // 按学生查询
        char stuNum[STU_NUM_LEN] = { 0 };
        safeReadString(stuNum, sizeof(stuNum), "输入学生学号: ");

        int studentIndex = findStudentIndex(stuNum);
        if (studentIndex == -1) {
            printf("该学生不存在！\n");
            return;
        }

        // 检查当前教师是否有权限查看该学生的成绩
        if (!teacherHasAccess(&students[studentIndex], currentUser.username)) {
            printf("错误：你不是该学生的授课教师，无权查看成绩！\n");
            return;
        }

        // 显示学生成绩
        printf("\n学生 %s 的成绩:\n", students[studentIndex].name);
        printStudentCourses(&students[studentIndex]);

        printf("平均分: %.1f\n", students[studentIndex].average);

    }
    else {
        // 按课程查询
        char courseName[30] = { 0 };
        safeReadString(courseName, sizeof(courseName), "输入课程名称: ");

        // 检查是否是当前教师教授的课程
        if (strcmp(courseName, currentUser.teachCourse) != 0) {
            printf("错误：你不是该课程的授课教师，无权查看成绩！\n");
            return;
        }

        printf("\n课程 %s 的成绩:\n", courseName);
        const int widths[] = {20, 16, 24, 10};
        const char* headers[] = {"学号", "姓名", "班级", "成绩"};
        tableStart(4, widths, headers);

        int hasScore = 0;
        for (int i = 0; i < studentCount; i++) {
            // 只显示当前教师有权限查看的学生
            if (!teacherHasAccess(&students[i], currentUser.username)) {
                continue;
            }

            int courseIndex = findCourseIndex(&students[i], courseName);
            if (courseIndex != -1) {
                int c = studentClass(students[i].stuNum);
                tableRow("sssf", students[i].stuNum, students[i].name,
                    c < 0 ? "未选择" : schoolClasses[c].name, (double)students[i].courses[courseIndex].score);
                hasScore = 1;
            }
        }

        tableEnd();
        if (!hasScore) {
            printf("暂无该课程的成绩记录！\n");
        }
    }
}

// 计算平均分
void calculateAverages() {
    int c = selectClass();
    if (c < 0) return;
    double sum = 0;
    int count = 0;
    for (int i = 0; i < studentCount; ++i) {
        if (studentClass(students[i].stuNum) != c || !teacherHasAccess(&students[i], currentUser.username)) continue;
        int j = findCourseIndex(&students[i], currentUser.teachCourse);
        if (j >= 0) { sum += students[i].courses[j].score; ++count; }
    }
    if (!count) { printf("所选班级暂无你有权限统计的该科目成绩！\n"); return; }
    printf("班级: %s 科目: %s 有成绩人数: %d 平均分: %.2f\n",
        schoolClasses[c].name, currentUser.teachCourse, count, sum/count);
}

// 成绩排名 - 修改后的函数
void rankStudents() {
    printf("\n[成绩排名方式]\n");
    printf("1. 按总分排名\n2. 按平均分排名\n3. 按各科最高分排名\n选择: ");
    int sortType = safeReadInt("", 1, 3);

    // 创建临时数组并复制学生数据
    Student tempStudents[MAX_STUDENTS];
    int tempCount = 0;

    // 只复制当前教师有权限查看的学生
    for (int i = 0; i < studentCount; i++) {
        if (teacherHasAccess(&students[i], currentUser.username)) {
            tempStudents[tempCount++] = students[i];
        }
    }

    if (tempCount == 0) {
        printf("没有找到你有权限查看的学生！\n");
        return;
    }

    // 排序函数
    for (int i = 0; i < tempCount - 1; i++) {
        for (int j = 0; j < tempCount - i - 1; j++) {
            int shouldSwap = 0;

            switch (sortType) {
            case 1: { // 按总分
                float total1 = calculateTotalScore(&tempStudents[j]);
                float total2 = calculateTotalScore(&tempStudents[j + 1]);
                shouldSwap = (total1 < total2);
                break;

            }
            case 2: // 按平均分
                shouldSwap = (tempStudents[j].average < tempStudents[j + 1].average);
                break;

            case 3: { // 按各科最高分
                float max1 = 0, max2 = 0;
                for (int k = 0; k < tempStudents[j].courseCount; k++) {
                    if (tempStudents[j].courses[k].score > max1) {
                        max1 = tempStudents[j].courses[k].score;
                    }
                }
                for (int k = 0; k < tempStudents[j + 1].courseCount; k++) {
                    if (tempStudents[j + 1].courses[k].score > max2) {
                        max2 = tempStudents[j + 1].courses[k].score;
                    }
                }
                shouldSwap = (max1 < max2);
                break;
            }
            }

            if (shouldSwap) {
                Student temp = tempStudents[j];
                tempStudents[j] = tempStudents[j + 1];
                tempStudents[j + 1] = temp;
            }
        }
    }

    // 显示排名结果
    printf("\n[学生成绩排名 - %s]\n",
        sortType == 1 ? "总分" :
        sortType == 2 ? "平均分" : "各科最高分");

    const int widths[] = {6, 20, 16, 10, 10, 20, 10};
    const char* headers[] = {"排名", "学号", "姓名", "总分", "平均分", "最高分科目", "最高分"};
    tableStart(7, widths, headers);
    for (int i = 0; i < tempCount; ++i) {
        Student* st = &tempStudents[i];
        int best = -1;
        for (int j = 0; j < st->courseCount; ++j)
            if (best < 0 || st->courses[j].score > st->courses[best].score) best = j;
        char total[32], average[32], high[32];
        if (best < 0) { strcpy(total, "--"); strcpy(average, "--"); strcpy(high, "--"); }
        else {
            snprintf(total, sizeof(total), "%.1f", calculateTotalScore(st));
            snprintf(average, sizeof(average), "%.1f", st->average);
            snprintf(high, sizeof(high), "%.1f", st->courses[best].score);
        }
        tableRow("dssssss", i+1, st->stuNum, st->name, total, average,
            best < 0 ? "--" : st->courses[best].course, high);
    }
    tableEnd();
}

// 显示及格率
void showPassRate() {
    char courseName[30] = { 0 };
    safeReadString(courseName, sizeof(courseName), "输入课程名称: ");

    // 检查是否是当前教师教授的课程
    if (strcmp(courseName, currentUser.teachCourse) != 0) {
        printf("错误：你不是该课程的授课教师，无权查看统计数据！\n");
        return;
    }

    int totalStudents = 0;
    int passStudents = 0;

    for (int i = 0; i < studentCount; i++) {
        // 只统计当前教师有权限查看的学生
        if (!teacherHasAccess(&students[i], currentUser.username)) {
            continue;
        }

        int courseIndex = findCourseIndex(&students[i], courseName);
        if (courseIndex != -1) {
            totalStudents++;
            if (students[i].courses[courseIndex].score >= PASS_SCORE) {
                passStudents++;
            }
        }
    }

    if (totalStudents == 0) {
        printf("暂无该课程的成绩记录！\n");
        return;
    }

    float passRate = (float)passStudents / totalStudents * 100;
    printf("\n课程 %s 的及格率: %.1f%% (%d/%d)\n",
        courseName, passRate, passStudents, totalStudents);
}

// CSV uses three unquoted fields: student ID, course, score.
static char* trimField(char* s) {
    while (isspace((unsigned char)*s)) ++s;
    char* end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) --end;
    *end = 0; return s;
}
void importScores(void) {
    int c = selectClass();
    if (c < 0) return;
    char path[512];
    safeReadString(path, sizeof(path), "CSV文件路径（UTF-8，不加引号）: ");
    wchar_t widePath[512];
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, widePath, 512)) {
        printf("路径编码无效！\n"); return;
    }
    FILE* file = _wfopen(widePath, L"rb");
    if (!file) { printf("无法打开导入文件！\n"); return; }
    Student* staged = malloc(sizeof(students));
    if (!staged) { fclose(file); printf("内存不足！\n"); return; }
    memcpy(staged, students, sizeof(students));
    int seen[MAX_STUDENTS] = {0};
    int lineNo = 0, imported = 0, valid = 1;
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        ++lineNo;
        if (!strchr(line, '\n') && !feof(file)) { valid = 0; break; }
        char* row = line;
        if (lineNo == 1 && !strncmp(row, "\xef\xbb\xbf", 3)) row += 3;
        row = trimField(row);
        if (!*row) continue;
        char* comma = strchr(row, ',');
        if (!comma) { valid = 0; break; }
        *comma++ = 0;
        char* second = strchr(comma, ',');
        if (!second || strchr(second+1, ',')) { valid = 0; break; }
        *second++ = 0;
        char* id = trimField(row);
        char* course = trimField(comma);
        char* scoreText = trimField(second);
        if (lineNo == 1 && ((!strcmp(id, "stuNum") && !strcmp(course, "course") && !strcmp(scoreText, "score")) ||
            (!strcmp(id, "学号") && !strcmp(course, "科目") && !strcmp(scoreText, "成绩")))) continue;
        char* end;
        errno = 0;
        float score = strtof(scoreText, &end);
        int i = findStudentIndex(id);
        if (!*scoreText || *end || errno || !isfinite(score) || score < 0 || score > 100 ||
            i < 0 || studentClass(id) != c || strcmp(course, currentUser.teachCourse) ||
            !teacherHasAccess(&students[i], currentUser.username) || seen[i]) { valid = 0; break; }
        int j = findCourseIndex(&staged[i], course);
        // Existing scores are protected; updates use the modify-scores menu.
        if (j >= 0 || staged[i].courseCount >= MAX_COURSES) { valid = 0; break; }
        j = staged[i].courseCount++;
        strcpy(staged[i].courses[j].course, course);
        staged[i].courses[j].score = score;
        seen[i] = 1; ++imported;
    }
    if (ferror(file)) valid = 0;
    fclose(file);
    if (!valid) printf("第%d行导入失败：检查格式、班级、科目、教师权限、重复成绩和分数范围；本次未导入任何成绩。\n", lineNo);
    else if (!imported) printf("文件中没有可导入的成绩。\n");
    else {
        memcpy(students, staged, sizeof(students));
        saveAll(); printf("成功导入 %d 条成绩。\n", imported);
    }
    free(staged);
}

// 教师菜单
void teacherMenu(void) {
    int choice;

    while (1) {
        clearScreen();
        printf("\n[教师菜单] - 当前用户: %s (教授科目: %s)\n",
            currentUser.username, currentUser.teachCourse);
        printf("1. 添加成绩\n2. 修改成绩\n3. 查询成绩\n");
        printf("4. 计算平均分\n5. 成绩排名\n6. 及格率统计\n");
        printf("7. 返回\n8. 文件导入成绩\n9. 查看所有学生\n10. 查看所有成绩\n选择: ");

        choice = safeReadInt("", 1, 10);

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
        }
        pauseScreen();
    }
}