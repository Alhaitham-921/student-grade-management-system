#include "maintenance.h"
int demoSeedVersion;
int migrateTeachers(void)
{
    int indices[MAX_USERS], n = 0, changed = 0;
    for (int i = 0; i < userCount; i++)
        if (users[i].type == TEACHER && !validTeacherId(users[i].username))
            indices[n++] = i;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (strcmp(users[indices[i]].username, users[indices[j]].username) > 0)
            {
                int x = indices[i];
                indices[i] = indices[j];
                indices[j] = x;
            }
    /* Preflight the entire mapping before modifying any record. */
    char ids[MAX_USERS][20], old[MAX_USERS][20];
    int next = 1;
    for (int i = 0; i < n; i++)
    {
        strcpy(old[i], users[indices[i]].username);
        do
        {
            if (next > 9999)
            {
                printf("教师工号空间不足，迁移未执行。\n");
                return 0;
            }
            snprintf(ids[i], 20, "T%04d", next++);
        } while (userExists(ids[i]));
    }
    for (int i = 0; i < n; i++)
    {
        int u = indices[i];
        strcpy(users[u].username, ids[i]);
        if (!strcmp(old[i], "111"))
            strcpy(userDisplayNames[u], "张三");
        else if (!validPersonName(userDisplayNames[u]) || !strcmp(userDisplayNames[u], old[i]))
            userDisplayNames[u][0] = 0;
        for (int s = 0; s < studentCount; s++)
            for (int j = 0; j < students[s].teacherCount; j++)
                if (!strcmp(students[s].teachers[j], old[i]))
                    strcpy(students[s].teachers[j], ids[i]);
        char detail[100];
        snprintf(detail, sizeof(detail), "old=%s new=%s", old[i], ids[i]);
        auditEvent("teacher-id-migration", detail);
        printf("教师工号迁移: %s -> %s (%s)\n", old[i], ids[i], displayName(ids[i]));
        changed = 1;
    }
    return !changed || saveAll();
}
typedef struct
{
    User u[MAX_USERS];
    Student s[MAX_STUDENTS];
    SchoolClass c[MAX_CLASSES];
    char names[MAX_USERS][60];
    int inactive[MAX_CLASSES];
    int nu, ns, nc;
} DemoState;
static const char *names[3][10] = {{"刘晨", "王悦", "陈宇", "赵敏", "周航", "吴桐", "徐欣", "孙睿", "胡宁", "何雨"},
                                   {"高远", "林佳", "郭帆", "马琳", "罗杰", "梁静", "宋阳", "郑雅", "谢然", "唐雪"},
                                   {"许诺", "韩松", "冯晴", "邓越", "曹月", "彭涛", "曾琪", "肖然", "田恬", "董博"}};
int seedDemoData(int confirmed)
{
    if (currentUser.type != ADMIN)
    {
        printf("无权初始化数据。\n");
        return 0;
    }
    if (demoSeedVersion == 1)
    {
        printf("示例数据已初始化，不重复创建或覆盖已有修改。\n");
        return 1;
    }
    int classes[3], newClasses = 0;
    const char *teacherNames[] = {"李华", "王芳", "陈明"};
    const char *courses[] = {"数学", "英语", "C语言"};
    for (int t = 0; t < 3; t++)
    {
        char id[20];
        snprintf(id, 20, "T%04d", t + 2);
        if (userExists(id))
        {
            printf("初始化冲突: 教师工号 %s 已存在，整批未写入。\n", id);
            return 0;
        }
    }
    for (int c = 0; c < 3; c++)
    {
        char name[60];
        snprintf(name, 60, "24级软件%d班", c + 1);
        classes[c] = -1;
        for (int j = 0; j < classCount; j++)
            if (!strcmp(name, schoolClasses[j].name))
            {
                classes[c] = j;
                if (classInactive[j])
                {
                    printf("初始化冲突: 班级 %s 已停用。\n", name);
                    return 0;
                }
            }
        if (classes[c] < 0)
            newClasses++;
        for (int j = 0; j < 10; j++)
        {
            char id[20];
            snprintf(id, 20, "24010%d%03d", c + 1, j + 1);
            if (userExists(id) || findStudentIndex(id) >= 0)
            {
                printf("初始化冲突: 学号 %s 已存在，整批未写入。\n", id);
                return 0;
            }
        }
        if (classes[c] >= 0 && schoolClasses[classes[c]].count + 10 > MAX_STUDENTS)
        {
            printf("班级容量不足。\n");
            return 0;
        }
    }
    if (userCount + 33 > MAX_USERS || studentCount + 30 > MAX_STUDENTS || classCount + newClasses > MAX_CLASSES)
    {
        printf("初始化容量不足，整批未写入。\n");
        return 0;
    }
    printf("初始化预览 新增教师: 3 班级: %d 复用班级: %d 学生: 30；保留管理员密码和现有数据。\n", newClasses,
           3 - newClasses);
    if (!confirmed && !confirmAction("1确认补充示例数据 / 0取消: "))
        return 0;
    DemoState *d = calloc(1, sizeof(*d));
    if (!d)
    {
        printf("内存不足。\n");
        return 0;
    }
    memcpy(d->u, users, sizeof(users));
    memcpy(d->s, students, sizeof(students));
    memcpy(d->c, schoolClasses, sizeof(schoolClasses));
    memcpy(d->names, userDisplayNames, sizeof(userDisplayNames));
    memcpy(d->inactive, classInactive, sizeof(classInactive));
    d->nu = userCount;
    d->ns = studentCount;
    d->nc = classCount;
    for (int t = 0; t < 3; t++)
    {
        User *u = &d->u[d->nu];
        snprintf(u->username, 20, "T%04d", t + 2);
        u->type = TEACHER;
        strcpy(u->teachCourse, courses[t]);
        hashPassword("Teacher123", u->password);
        strcpy(d->names[d->nu++], teacherNames[t]);
    }
    for (int c = 0; c < 3; c++)
    {
        if (classes[c] < 0)
        {
            classes[c] = d->nc++;
            memset(&d->c[classes[c]], 0, sizeof(SchoolClass));
            snprintf(d->c[classes[c]].name, 60, "24级软件%d班", c + 1);
            d->inactive[classes[c]] = 0;
        }
        for (int j = 0; j < 10; j++)
        {
            Student *s = &d->s[d->ns++];
            memset(s, 0, sizeof(*s));
            snprintf(s->stuNum, 20, "24010%d%03d", c + 1, j + 1);
            strcpy(s->name, names[c][j]);
            s->teacherCount = 3;
            for (int t = 0; t < 3; t++)
            {
                snprintf(s->teachers[t], 20, "T%04d", t + 2);
                if (j == 9 && t == 2)
                    continue;
                Course *score = &s->courses[s->courseCount++];
                strcpy(score->course, courses[t]);
                score->score = (float)(60 + ((c + 1) * 7 + (j + 1) * 3 + (t + 1) * 5) % 41);
                if (t == 0)
                {
                    if (j == 0)
                        score->score = 0;
                    if (j == 1 || j == 2)
                        score->score = 80;
                    if (j == 3)
                        score->score = 100;
                }
            }
            User *u = &d->u[d->nu];
            memset(u, 0, sizeof(*u));
            strcpy(u->username, s->stuNum);
            u->type = STUDENT;
            hashPassword(s->stuNum, u->password);
            strcpy(d->names[d->nu++], s->name);
            SchoolClass *school = &d->c[classes[c]];
            strcpy(school->members[school->count++], s->stuNum);
        }
    }
    memcpy(users, d->u, sizeof(users));
    memcpy(students, d->s, sizeof(students));
    memcpy(schoolClasses, d->c, sizeof(schoolClasses));
    memcpy(userDisplayNames, d->names, sizeof(userDisplayNames));
    memcpy(classInactive, d->inactive, sizeof(classInactive));
    userCount = d->nu;
    studentCount = d->ns;
    classCount = d->nc;
    free(d);
    demoSeedVersion = 1;
    auditEvent("demo-initialize", "teachers=3 students=30 classes=3");
    if (!saveAll())
        return 0;
    printf("示例数据初始化成功：新增3位教师、30名学生。\n");
    return 1;
}
void seedDemoMenu(void)
{
    seedDemoData(0);
}
