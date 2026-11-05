#include "maintenance.h"
static int eligible(int i, int c)
{
    return currentUser.type == TEACHER && studentClass(students[i].stuNum) == c && hasTeacherAccess(&students[i]);
}
void classStatistics(void)
{
    if (currentUser.type != TEACHER)
        return;
    int c = selectClass();
    if (c < 0)
        return;
    int valid = 0, missing = 0, pass = 0;
    double sum = 0, lo = 100, hi = 0;
    for (int i = 0; i < studentCount; i++)
        if (eligible(i, c))
        {
            int j = courseIndex(&students[i], currentUser.teachCourse);
            if (j < 0)
            {
                missing++;
                continue;
            }
            double v = students[i].courses[j].score;
            valid++;
            sum += v;
            if (v < lo)
                lo = v;
            if (v > hi)
                hi = v;
            if (v >= PASS_SCORE)
                pass++;
        }
    printf("班级: %s 科目: %s 有成绩人数: %d 未录入人数: %d\n", schoolClasses[c].name, currentUser.teachCourse, valid,
           missing);
    if (!valid)
    {
        printf("平均分: -- 最高分: -- 最低分: -- 及格率: --\n");
        return;
    }
    printf("平均分: %.2f 最高分: %.2f 最低分: %.2f 及格率: %.2f%% (%d/%d)\n", sum / valid, hi, lo, pass * 100.0 / valid,
           pass, valid);
}
void classRanking(void)
{
    if (currentUser.type != TEACHER)
        return;
    int c = selectClass();
    if (c < 0)
        return;
    int indices[MAX_STUDENTS], n = 0;
    float values[MAX_STUDENTS];
    for (int i = 0; i < studentCount; i++)
        if (eligible(i, c))
        {
            int j = courseIndex(&students[i], currentUser.teachCourse);
            if (j >= 0)
            {
                indices[n] = i;
                values[n++] = students[i].courses[j].score;
            }
        }
    for (int i = 1; i < n; i++)
    {
        int index = indices[i], j = i;
        float v = values[i];
        while (j > 0 && (values[j - 1] < v ||
                         (values[j - 1] == v && strcmp(students[indices[j - 1]].stuNum, students[index].stuNum) > 0)))
        {
            values[j] = values[j - 1];
            indices[j] = indices[j - 1];
            j--;
        }
        values[j] = v;
        indices[j] = index;
    }
    const int w[] = {6, 20, 16, 24, 10};
    const char *h[] = {"排名", "学号", "姓名", "科目", "成绩"};
    tableStart(5, w, h);
    int rank = 0;
    for (int i = 0; i < n; i++)
    {
        if (!i || values[i] != values[i - 1])
            rank = i + 1;
        Student *s = &students[indices[i]];
        tableRow("dsssf", rank, s->stuNum, s->name, currentUser.teachCourse, (double)values[i]);
        if ((i + 1) % 10 == 0)
        {
            tableEnd();
            if (!pageNext(i + 1))
                return;
            tableStart(5, w, h);
        }
    }
    tableEnd();
    printf("参与排名人数: %d；未录入不参与排名。\n", n);
}
void classQuery(void)
{
    if (currentUser.type != TEACHER)
        return;
    int c = selectClass();
    if (c < 0)
        return;
    int mode = safeReadInt("1按学生查看完整成绩 / 2当前任教科目 / 0取消: ", 0, 2);
    if (!mode)
        return;
    if (mode == 1)
    {
        char id[20];
        safeReadString(id, sizeof(id), "学号 (0取消): ");
        if (!strcmp(id, "0"))
            return;
        int s = findStudentIndex(id);
        if (s < 0 || !eligible(s, c))
        {
            printf("该学生不属于所选班级或无授课权限。\n");
            return;
        }
        printf("学生: %s；授权教师查看完整成绩\n", students[s].name);
        printStudentCourses(&students[s]);
        return;
    }
    const int w[] = {20, 16, 24, 10};
    const char *h[] = {"学号", "姓名", "班级", "成绩"};
    tableStart(4, w, h);
    int n = 0;
    for (int i = 0; i < studentCount; i++)
        if (eligible(i, c))
        {
            int j = courseIndex(&students[i], currentUser.teachCourse);
            char score[32] = "--";
            if (j >= 0)
                snprintf(score, sizeof(score), "%.1f", students[i].courses[j].score);
            tableRow("ssss", students[i].stuNum, students[i].name, schoolClasses[c].name, score);
            if (++n % 10 == 0)
            {
                tableEnd();
                if (!pageNext(n))
                    return;
                tableStart(4, w, h);
            }
        }
    tableEnd();
}
