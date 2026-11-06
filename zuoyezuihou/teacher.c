#include "teacher.h"
#include "maintenance.h"
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <windows.h>

// 添加成绩
void addScores(void)
{
    char stuNum[STU_NUM_LEN] = {0};

    if (!requireTeacherStudents())
        return;
    printf("\n[添加成绩]\n");
    int selectedClass = selectClass();
    if (selectedClass < 0)
        return;
    if (classInactive[selectedClass])
    {
        printf("班级已停用，不能添加新成绩。\n");
        return;
    }
    printf("班级学生（你有授课权限）:\n");
    const int widths[] = {20, 16};
    const char *headers[] = {"学号", "姓名"};
    tableStart(2, widths, headers);
    int visible = 0;
    for (int i = 0; i < studentCount; ++i)
        if (studentClass(students[i].stuNum) == selectedClass && hasTeacherAccess(&students[i]))
        {
            tableRow("ss", students[i].stuNum, students[i].name);
            visible++;
        }
    tableEnd();
    if (!visible)
    {
        printf("本班级没有你有授课权限的学生，无权添加成绩；请联系管理员分配。\n");
        return;
    }
    if (readStudentId(stuNum, sizeof(stuNum), "学生学号 (:q取消): ") != INPUT_OK)
        return;

    int studentIndex = findStudentIndex(stuNum);
    if (studentIndex == -1)
    {
        printf("该学生不存在，请先在管理员端添加学生用户\n");
        return;
    }

    if (studentClass(stuNum) != selectedClass)
    {
        printf("该学生不属于所选班级！\n");
        return;
    }
    // 检查当前教师是否有权限添加该学生的成绩
    if (!hasTeacherAccess(&students[studentIndex]))
    {
        printf("错误：你不是该学生的授课教师，无权添加成绩！\n");
        return;
    }

    Student *student = &students[studentIndex];
    if (courseIndex(student, currentUser.teachCourse) >= 0)
    {
        printf("该课程已有成绩，请使用修改成绩功能！\n");
        return;
    }
    if (student->courseCount >= MAX_COURSES)
    {
        printf("课程数量已达上限！\n");
        return;
    }
    float score = safeReadFloat("输入课程成绩 (0-100): ", 0, 100);
    if (!isfinite(score))
        return;
    int index = student->courseCount++;
    strcpy(student->courses[index].course, currentUser.teachCourse);
    student->courses[index].score = score;
    if (saveAll())
        printf("成绩添加完成！\n");
}

// 修改成绩
void modifyScores(void)
{
    if (!requireTeacherStudents())
        return;
    printf("\n[修改成绩]\n");

    // 收集当前教师有权限修改成绩的学生列表
    int teacherStudents[MAX_STUDENTS];
    int teacherStudentCount = 0;

    for (int i = 0; i < studentCount; i++)
    {
        if (hasTeacherAccess(&students[i]))
        {
            teacherStudents[teacherStudentCount++] = i;
        }
    }

    if (teacherStudentCount == 0)
    {
        printf("你没有可修改成绩的学生！\n");
        return;
    }

    // 显示学生列表
    printf("你所教的学生列表:\n");
    const int widths[] = {6, 20, 16};
    const char *headers[] = {"序号", "学号", "姓名"};
    tableStart(3, widths, headers);
    for (int i = 0; i < teacherStudentCount; ++i)
    {
        int index = teacherStudents[i];
        tableRow("dss", i + 1, students[index].stuNum, students[index].name);
    }
    tableEnd();

    // 选择学生
    printf("%d. 返回上一级\n", teacherStudentCount + 1);
    int selection = safeReadInt("选择学生序号: ", 1, teacherStudentCount + 1) - 1;
    if (selection < 0 || selection == teacherStudentCount)
        return;
    int studentIndex = teacherStudents[selection];

    if (students[studentIndex].courseCount == 0)
    {
        printf("该学生暂无成绩记录！\n");
        return;
    }
    // 显示学生已有成绩
    printf("\n学生 %s (学号: %s) 的成绩:\n", students[studentIndex].name, students[studentIndex].stuNum);
    printStudentCourses(&students[studentIndex]);

    // 检查是否有成绩可修改
    if (students[studentIndex].courseCount == 0)
    {
        printf("该学生暂无成绩记录！\n");
        return;
    }

    // 选择要修改的课程
    printf("%d. 返回上一级\n", students[studentIndex].courseCount + 1);
    int courseIndex = safeReadInt("选择要修改的课程序号: ", 1, students[studentIndex].courseCount + 1) - 1;
    if (courseIndex < 0 || courseIndex == students[studentIndex].courseCount)
        return;

    // 检查课程是否属于当前教师所授科目
    if (strcmp(students[studentIndex].courses[courseIndex].course, currentUser.teachCourse) != 0)
    {
        printf("错误：该课程不是你所教授的科目！\n");
        return;
    }

    // 修改成绩
    float newScore = safeReadFloat("输入新成绩: ", 0.0f, 100.0f);
    if (!isfinite(newScore))
        return;
    students[studentIndex].courses[courseIndex].score = newScore;

    if (saveAll())
        printf("成绩修改成功！\n");
}

// 查询成绩
void queryScores(void)
{
    classQuery();
}

// 计算平均分
void calculateAverages(void)
{
    classStatistics();
}

// 成绩排名 - 修改后的函数
void rankStudents(void)
{
    classRanking();
}

// 显示及格率
void showPassRate(void)
{
    classStatistics();
}

// CSV validation, preview and atomic import live in csv.c.
void importScores(void)
{
    csvImport();
}

#include "admin.h"
static void teacherStudents(void)
{
    filteredList(0);
}
void teacherMenu(void)
{
    if (currentUser.type != TEACHER)
        return;
    const MenuItem m[] = {{"手工录入成绩", addScores},
                          {"修改已有成绩", modifyScores},
                          {"CSV批量导入", csvImport},
                          {"按班级查询成绩", classQuery},
                          {"班级成绩统计", classStatistics},
                          {"成绩排名", classRanking},
                          {"导出CSV", csvExport},
                          {"查看授权学生", teacherStudents},
                          {"搜索学生", searchRecords},
                          {"修改密码", changePassword},
                          {"退出登录", NULL}};
    runMenu("教师菜单", m, 11);
}
