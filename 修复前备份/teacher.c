#include "teacher.h"

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
    safeReadString(stuNum, sizeof(stuNum), "输入学生学号: ");

    int studentIndex = findStudentIndex(stuNum);
    if (studentIndex == -1) {
        printf("该学生不存在，请先在管理员端添加学生用户\n");
        return;
    }

    // 检查当前教师是否有权限添加该学生的成绩
    if (!teacherHasAccess(&students[studentIndex], currentUser.username)) {
        printf("错误：你不是该学生的授课教师，无权添加成绩！\n");
        return;
    }

    // 确保课程数量有效
    if (students[studentIndex].courseCount < 0 ||
        students[studentIndex].courseCount >= MAX_COURSES) {
        students[studentIndex].courseCount = 0;
    }

    printf("学生 %s 的当前课程数量: %d\n",
        students[studentIndex].name, students[studentIndex].courseCount);

    int availableSlots = MAX_COURSES - students[studentIndex].courseCount;

    if (availableSlots <= 0) {
        printf("该学生的课程数量已达上限(%d)\n", MAX_COURSES);
        return;
    }

    int courseNum = safeReadInt("输入要添加的课程数量(1-%d): ", 1, availableSlots);

    for (int i = 0; i < courseNum; i++) {
        char courseName[30] = { 0 };
        float courseScore;

        // 读取课程名称（自动填充为教师所授科目）
        strcpy(courseName, currentUser.teachCourse);
        printf("课程%d名称: %s\n", i + 1, courseName);

        // 读取课程成绩
        char prompt[50] = { 0 };
        sprintf(prompt, "课程%d成绩: ", i + 1);
        courseScore = safeReadFloat(prompt, 0.0f, 100.0f);

        // 添加课程成绩
        int idx = students[studentIndex].courseCount;

        // 检查索引是否有效
        if (idx >= MAX_COURSES) {
            printf("错误：课程数量超出上限！\n");
            break;
        }

        strcpy(students[studentIndex].courses[idx].course, courseName);
        students[studentIndex].courses[idx].score = courseScore;
        students[studentIndex].courseCount++;

        printf("课程 %s (成绩: %.1f) 添加成功！\n", courseName, courseScore);
    }

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
    printf("序号  学号            姓名\n");
    printf("---------------------------\n");

    for (int i = 0; i < teacherStudentCount; i++) {
        int studentIndex = teacherStudents[i];
        printf("%-5d %-16s %-10s\n",
            i + 1,
            students[studentIndex].stuNum,
            students[studentIndex].name);
    }

    // 选择学生
    int selection = safeReadInt("选择学生序号(1-%d): ", 1, teacherStudentCount) - 1;
    int studentIndex = teacherStudents[selection];

    // 显示学生已有成绩
    printf("\n学生 %s (学号: %s) 的成绩:\n",
        students[studentIndex].name,
        students[studentIndex].stuNum);
    printf("序号  课程                成绩\n");
    printf("------------------------------\n");

    for (int j = 0; j < students[studentIndex].courseCount; j++) {
        printf("%-5d %-20s %.1f\n",
            j + 1,
            students[studentIndex].courses[j].course,
            students[studentIndex].courses[j].score);
    }

    // 检查是否有成绩可修改
    if (students[studentIndex].courseCount == 0) {
        printf("该学生暂无成绩记录！\n");
        return;
    }

    // 选择要修改的课程
    int courseIndex = safeReadInt("选择要修改的课程序号(1-%d): ", 1, students[studentIndex].courseCount) - 1;

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
        printf("课程                成绩\n");
        printf("-------------------------\n");

        for (int i = 0; i < students[studentIndex].courseCount; i++) {
            printf("%-20s %.1f\n",
                students[studentIndex].courses[i].course,
                students[studentIndex].courses[i].score);
        }

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
        printf("学生姓名            成绩\n");
        printf("-------------------------\n");

        int hasScore = 0;
        for (int i = 0; i < studentCount; i++) {
            // 只显示当前教师有权限查看的学生
            if (!teacherHasAccess(&students[i], currentUser.username)) {
                continue;
            }

            int courseIndex = findCourseIndex(&students[i], courseName);
            if (courseIndex != -1) {
                printf("%-16s %.1f\n",
                    students[i].name,
                    students[i].courses[courseIndex].score);
                hasScore = 1;
            }
        }

        if (!hasScore) {
            printf("暂无该课程的成绩记录！\n");
        }
    }
}

// 计算平均分
void calculateAverages() {
    for (int i = 0; i < studentCount; i++) {
        // 只计算当前教师有权限查看的学生
        if (!teacherHasAccess(&students[i], currentUser.username)) {
            continue;
        }

        float sum = 0;
        for (int j = 0; j < students[i].courseCount; j++) {
            sum += students[i].courses[j].score;
        }
        students[i].average = (students[i].courseCount > 0) ? sum / students[i].courseCount : 0;
    }
    printf("平均分计算完成！\n");
    saveAll();
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
            case 1: // 按总分
                float total1 = calculateTotalScore(&tempStudents[j]);
                float total2 = calculateTotalScore(&tempStudents[j + 1]);
                shouldSwap = (total1 < total2);
                break;

            case 2: // 按平均分
                shouldSwap = (tempStudents[j].average < tempStudents[j + 1].average);
                break;

            case 3: // 按各科最高分
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

    if (sortType == 1) {
        // 按总分排名时显示详细科目
        printf("排名  学号            姓名            总分\n");
        printf("--------------------------------------------\n");

        for (int i = 0; i < tempCount; i++) {
            float total = calculateTotalScore(&tempStudents[i]);
            printf("%-4d %-16s %-16s %.1f\n",
                i + 1,
                tempStudents[i].stuNum,
                tempStudents[i].name,
                total);

            // 显示科目详情
            printf("  科目成绩明细:\n");
            for (int j = 0; j < tempStudents[i].courseCount; j++) {
                printf("    %-16s %.1f\n",
                    tempStudents[i].courses[j].course,
                    tempStudents[i].courses[j].score);
            }
            printf("--------------------------------------------\n");
        }
    }
    else if (sortType == 2) {
        // 按平均分排名显示基本信息
        printf("排名  学号            姓名            平均分\n");
        printf("--------------------------------------------\n");

        for (int i = 0; i < tempCount; i++) {
            printf("%-4d %-16s %-16s %.1f\n",
                i + 1,
                tempStudents[i].stuNum,
                tempStudents[i].name,
                tempStudents[i].average);
        }
    }
    else {
        // 按各科最高分显示详情
        printf("排名  学号            姓名            最高分科目       最高分\n");
        printf("--------------------------------------------------------\n");

        for (int i = 0; i < tempCount; i++) {
            float maxScore = 0;
            char maxCourse[30] = "";

            for (int k = 0; k < tempStudents[i].courseCount; k++) {
                if (tempStudents[i].courses[k].score > maxScore) {
                    maxScore = tempStudents[i].courses[k].score;
                    strcpy(maxCourse, tempStudents[i].courses[k].course);
                }
            }

            printf("%-4d %-16s %-16s %-16s %.1f\n",
                i + 1,
                tempStudents[i].stuNum,
                tempStudents[i].name,
                maxCourse,
                maxScore);
        }
    }
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

// 教师菜单
void teacherMenu(void) {
    int choice;

    while (1) {
        system("cls");
        printf("\n[教师菜单] - 当前用户: %s (教授科目: %s)\n",
            currentUser.username, currentUser.teachCourse);
        printf("1. 添加成绩\n2. 修改成绩\n3. 查询成绩\n");
        printf("4. 计算平均分\n5. 成绩排名\n6. 及格率统计\n");
        printf("7. 返回\n选择: ");

        choice = safeReadInt("", 1, 7);

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
        }
        system("pause");
    }
}