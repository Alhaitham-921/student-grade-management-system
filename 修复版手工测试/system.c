#include "system.h"
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <io.h>
#include <time.h>
#include <windows.h>
#include <bcrypt.h>
#include "admin.h"
#pragma comment(lib, "bcrypt.lib")

static void derivePassword(const char* plain, const unsigned char* salt, unsigned char* result) {
    BCRYPT_ALG_HANDLE alg = NULL;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0)
        exit(EXIT_FAILURE);
    NTSTATUS status = BCryptDeriveKeyPBKDF2(alg, (PUCHAR)plain, (ULONG)strlen(plain),
        (PUCHAR)salt, 16, 100000, result, 32, 0);
    BCryptCloseAlgorithmProvider(alg, 0);
    if (status < 0) exit(EXIT_FAILURE);
}
void hashPassword(const char* plain, char* encoded) {
    unsigned char salt[16], digest[32];
    if (BCryptGenRandom(NULL, salt, 16, BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) exit(EXIT_FAILURE);
    derivePassword(plain, salt, digest);
    strcpy(encoded, "p1$");
    for (int i = 0; i < 16; ++i) sprintf(encoded + 3 + 2*i, "%02x", salt[i]);
    for (int i = 0; i < 32; ++i) sprintf(encoded + 35 + 2*i, "%02x", digest[i]);
}
int verifyPassword(const char* plain, const char* encoded) {
    if (strlen(encoded) != 99 || strncmp(encoded, "p1$", 3)) return 0;
    unsigned char salt[16], digest[32];
    char actual[65];
    for (int i = 0; i < 16; ++i) {
        unsigned int value;
        if (sscanf(encoded + 3 + 2*i, "%2x", &value) != 1) return 0;
        salt[i] = (unsigned char)value;
    }
    derivePassword(plain, salt, digest);
    for (int i = 0; i < 32; ++i) sprintf(actual + 2*i, "%02x", digest[i]);
    unsigned int difference = 0;
    for (int i = 0; i < 64; ++i) difference |= (unsigned char)actual[i] ^ (unsigned char)encoded[35+i];
    return difference == 0;
}
User users[MAX_USERS];
int userCount = 0;
Student students[MAX_STUDENTS];
int studentCount = 0;
User currentUser;
SchoolClass schoolClasses[MAX_CLASSES];
int classCount = 0;

// Read one complete line; never leave excess input for the next prompt.
static void readLine(char* buffer, size_t size, const char* prompt) {
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!fgets(buffer, (int)size, stdin)) exit(0);
        char* newline = strchr(buffer, '\n');
        if (newline) { *newline = 0; return; }
        int ch = getchar();
        if (ch == '\n' || ch == EOF) return;
        while (ch != '\n' && ch != EOF) ch = getchar();
        printf("输入过长，请重新输入！\n");
    }
}
void safeReadString(char* buffer, size_t size, const char* prompt) {
    for (;;) {
        readLine(buffer, size, prompt);
        int nonempty = 0;
        for (size_t i = 0; buffer[i]; ++i)
            if (!isspace((unsigned char)buffer[i])) nonempty = 1;
        if (nonempty) return;
        printf("输入不能为空！\n");
    }
}
int safeReadInt(const char* prompt, int min, int max) {
    char buffer[100], *end;
    for (;;) {
        readLine(buffer, sizeof(buffer), prompt);
        errno = 0;
        long value = strtol(buffer, &end, 10);
        int valid = end != buffer;
        while (isspace((unsigned char)*end)) ++end;
        if (valid && !*end && !errno && value >= min && value <= max) return (int)value;
        printf("请输入 %d 到 %d 之间的整数。\n", min, max);
    }
}
float safeReadFloat(const char* prompt, float min, float max) {
    char buffer[100], *end;
    for (;;) {
        readLine(buffer, sizeof(buffer), prompt);
        errno = 0;
        float value = strtof(buffer, &end);
        int valid = end != buffer;
        while (isspace((unsigned char)*end)) ++end;
        if (valid && !*end && !errno && isfinite(value) && value >= min && value <= max) return value;
        printf("请输入 %.1f 到 %.1f 之间的分数。\n", min, max);
    }
}
void clearScreen(void) {
    if (_isatty(_fileno(stdin))) system("cls");
}
void pauseScreen(void) {
    if (_isatty(_fileno(stdin))) {
        char buffer[8]; readLine(buffer, sizeof(buffer), "按回车继续...");
    }
}
void refreshAverages(void) {
    for (int i = 0; i < studentCount; ++i) {
        float sum = 0;
        for (int j = 0; j < students[i].courseCount; ++j) sum += students[i].courses[j].score;
        students[i].average = students[i].courseCount ? sum / students[i].courseCount : 0;
    }
}
// 初始化系统，创建默认管理员账户
void initSystem() {
    if (userCount == 0) {
        User admin = { 0 };
        strcpy(admin.username, "admin");
        hashPassword("admin", admin.password);
        admin.type = ADMIN;
        users[userCount++] = admin;
        saveAll();
        printf("初始管理员账户已创建（用户名: admin, 密码: admin）\n");
        printf("请登录后修改默认密码！\n");
    }
}

static void migrateText(char* text, size_t capacity) {
    wchar_t wide[256]; char utf8[768];
    if (!MultiByteToWideChar(936, MB_ERR_INVALID_CHARS, text, -1, wide, 256) ||
        !WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, sizeof(utf8), NULL, NULL) ||
        strlen(utf8) >= capacity) {
        fprintf(stderr, "旧数据文本转换失败或过长，请根据备份人工恢复。\n"); exit(EXIT_FAILURE);
    }
    strcpy(text, utf8);
}
// Reject corrupt files before exposing their contents to the application.
static void dataError(const char* path) {
    fprintf(stderr, "数据文件 %s 损坏或不兼容。请恢复备份；未覆盖原文件。\n", path);
    exit(EXIT_FAILURE);
}
static void loadRecords(const char* path, void* records, size_t stride, int* count, int limit) {
    FILE* file = fopen(path, "rb");
    *count = 0;
    if (!file) { if (errno != ENOENT) dataError(path); return; }
    int n;
    if (fread(&n, sizeof(n), 1, file) != 1 || n < 0 || n > limit ||
        fread(records, stride, n, file) != (size_t)n || fgetc(file) != EOF || ferror(file)) {
        fclose(file); dataError(path);
    }
    fclose(file); *count = n;
}
#define TERMINATED(x) (memchr((x), 0, sizeof(x)) != NULL)
void loadAll(void) {
    // Detect and migrate the original 76-byte User records.
    FILE* legacy = fopen("users.dat", "rb");
    int oldFormat = 0;
    if (legacy) {
        int count = -1;
        fread(&count, sizeof(count), 1, legacy);
        fseek(legacy, 0, SEEK_END);
        oldFormat = count > 0 && count <= MAX_USERS && ftell(legacy) == 4L + 76L * count;
        rewind(legacy);
        if (oldFormat) {
            typedef struct { char username[20], password[20]; UserType type; char course[30]; } LegacyUser;
            fread(&userCount, sizeof(userCount), 1, legacy);
            for (int i = 0; i < userCount; ++i) {
                LegacyUser old;
                if (fread(&old, sizeof(old), 1, legacy) != 1 || !TERMINATED(old.username) ||
                    !TERMINATED(old.password) || !TERMINATED(old.course)) dataError("users.dat");
                strcpy(users[i].username, old.username);
                hashPassword(old.password, users[i].password);
                users[i].type = old.type;
                strcpy(users[i].teachCourse, old.course);
            }
        }
        fclose(legacy);
    }
    if (!oldFormat) loadRecords("users.dat", users, sizeof(User), &userCount, MAX_USERS);
    loadRecords("students.dat", students, sizeof(Student), &studentCount, MAX_STUDENTS);
    for (int i = 0; i < userCount; ++i) {
        if (!TERMINATED(users[i].username) || !TERMINATED(users[i].password) ||
            !TERMINATED(users[i].teachCourse) || users[i].type < ADMIN || users[i].type > STUDENT)
            dataError("users.dat");
    }
    for (int i = 0; i < studentCount; ++i) {
        Student* st = &students[i];
        if (!TERMINATED(st->stuNum) || !TERMINATED(st->name) || st->courseCount < 0 ||
            st->courseCount > MAX_COURSES || st->teacherCount < 0 || st->teacherCount > MAX_TEACHERS_PER_STUDENT)
            dataError("students.dat");
        for (int j = 0; j < st->teacherCount; ++j)
            if (!TERMINATED(st->teachers[j])) dataError("students.dat");
        for (int j = 0; j < st->courseCount; ++j)
            if (!TERMINATED(st->courses[j].course) || !isfinite(st->courses[j].score) ||
                st->courses[j].score < 0 || st->courses[j].score > 100) dataError("students.dat");
    }
    if (oldFormat) {
        for (int i = 0; i < userCount; ++i) {
            migrateText(users[i].username, sizeof(users[i].username));
            migrateText(users[i].teachCourse, sizeof(users[i].teachCourse));
        }
        for (int i = 0; i < studentCount; ++i) {
            migrateText(students[i].name, sizeof(students[i].name));
            for (int j = 0; j < students[i].teacherCount; ++j)
                migrateText(students[i].teachers[j], sizeof(students[i].teachers[j]));
            for (int j = 0; j < students[i].courseCount; ++j)
                migrateText(students[i].courses[j].course, sizeof(students[i].courses[j].course));
        }
    }
    // Legacy student accounts used names. Migrate only unambiguous matches.
    for (int i = 0; i < userCount; ++i) {
        if (users[i].type != STUDENT || findStudentIndex(users[i].username) >= 0) continue;
        int match = -1;
        for (int j = 0; j < studentCount; ++j) {
            if (strcmp(students[j].name, users[i].username) == 0) {
                if (match != -1) dataError("users.dat (重名账号需人工恢复)");
                match = j;
            }
        }
        if (match < 0) {
            printf("旧学生账号 %s 没有对应档案，已禁用该账号登录。\n", users[i].username);
            continue;
        }
        if (userExists(students[match].stuNum)) dataError("users.dat (冲突账号)");
        strcpy(users[i].username, students[match].stuNum);
    }
    for (int j = 0; j < studentCount; ++j) {
        if (userExists(students[j].stuNum)) continue;
        if (userCount >= MAX_USERS) dataError("users.dat (无法恢复缺失账号)");
        User restored = {0};
        strcpy(restored.username, students[j].stuNum);
        hashPassword(students[j].stuNum, restored.password);
        restored.type = STUDENT;
        users[userCount++] = restored;
        printf("已恢复学号 %s 的缺失账号，初始密码为学号。\n", restored.username);
    }
    loadRecords("classes.dat", schoolClasses, sizeof(SchoolClass), &classCount, MAX_CLASSES);
    for (int i = 0; i < classCount; ++i) {
        SchoolClass* c = &schoolClasses[i];
        if (!TERMINATED(c->name) || !c->name[0] || c->count < 0 || c->count > MAX_STUDENTS) dataError("classes.dat");
        for (int k = 0; k < i; ++k)
            if (!strcmp(c->name, schoolClasses[k].name)) dataError("classes.dat");
        for (int j = 0; j < c->count; ++j) {
            if (!TERMINATED(c->members[j]) || findStudentIndex(c->members[j]) < 0) dataError("classes.dat");
            for (int k = 0; k <= i; ++k)
                for (int m = 0; m < (k == i ? j : schoolClasses[k].count); ++m)
                    if (!strcmp(c->members[j], schoolClasses[k].members[m])) dataError("classes.dat");
        }
    }
    refreshAverages();
}
static void saveRecords(const char* path, const void* records, size_t stride, int count) {
    char temp[64];
    sprintf(temp, "%s.tmp", path);
    FILE* file = fopen(temp, "wb");
    if (!file) { fprintf(stderr, "无法写入 %s，请检查目录权限。\n", path); exit(EXIT_FAILURE); }
    int ok = fwrite(&count, sizeof(count), 1, file) == 1 &&
        fwrite(records, stride, count, file) == (size_t)count;
    if (fclose(file) != 0) ok = 0;
    if (!ok || !MoveFileExA(temp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        fprintf(stderr, "保存 %s 失败，原文件未覆盖。\n", path); exit(EXIT_FAILURE);
    }
}
void saveAll(void) {
    refreshAverages();
    saveRecords("users.dat", users, sizeof(User), userCount);
    saveRecords("students.dat", students, sizeof(Student), studentCount);
    saveRecords("classes.dat", schoolClasses, sizeof(SchoolClass), classCount);
}

// 检查用户是否存在
int userExists(const char* username) {
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return 1;
        }
    }
    return 0;
}

// 根据学号查找学生索引
int findStudentIndex(const char* stuNum) {
    for (int i = 0; i < studentCount; i++) {
        if (strcmp(students[i].stuNum, stuNum) == 0) {
            return i;
        }
    }
    return -1;
}

// 根据用户名查找学生
Student* findStudentByUsername(const char* username) {
    for (int i = 0; i < studentCount; i++) {
        if (strcmp(students[i].stuNum, username) == 0) {
            return &students[i];
        }
    }
    return NULL;
}

// 登录函数
int login(UserType requiredType) {
    char username[20];
    char password[20];
    static time_t lockedUntil = 0;
    if (time(NULL) < lockedUntil) {
        printf("登录暂时锁定，请一分钟后重试。\n"); pauseScreen(); return 0;
    }
    int attempts = LOGIN_ATTEMPTS;

    while (attempts > 0) {
        printf("\n[登录 - %s用户]\n",
            requiredType == ADMIN ? "管理员" :
            requiredType == TEACHER ? "教师" : "学生");
        safeReadString(username, sizeof(username), "用户名: ");
        safeReadString(password, sizeof(password), "密码: ");

        for (int i = 0; i < userCount; i++) {
            if (strcmp(users[i].username, username) == 0 &&
                verifyPassword(password, users[i].password) &&
                users[i].type == requiredType) {
                if (requiredType == STUDENT && !findStudentByUsername(username)) {
                    printf("账号没有对应学生档案，请联系管理员。\n"); return 0;
                }
                currentUser = users[i];
                if (requiredType == ADMIN && verifyPassword("admin", currentUser.password)) {
                    printf("首次登录必须修改默认密码（至少6个字节）。\n");
                    do { changePassword(); } while (verifyPassword("admin", currentUser.password));
                }
                return 1;
            }
        }

        attempts--;
        printf("用户名、密码或用户类型错误，你还有 %d 次尝试机会。\n", attempts);
    }

    lockedUntil = time(NULL) + 60;
    printf("登录失败，锁定一分钟！\n");
    return 0;
}
int studentClass(const char* id) {
    for (int i = 0; i < classCount; ++i)
        for (int j = 0; j < schoolClasses[i].count; ++j)
            if (!strcmp(id, schoolClasses[i].members[j])) return i;
    return -1;
}
void removeClassMember(const char* id) {
    for (int i = 0; i < classCount; ++i) {
        SchoolClass* c = &schoolClasses[i];
        for (int j = 0; j < c->count; ++j) {
            if (strcmp(id, c->members[j])) continue;
            memmove(c->members[j], c->members[j+1], (c->count-j-1)*STU_NUM_LEN);
            --c->count; break;
        }
    }
}
void listClasses(void) {
    const int widths[] = {6, 40, 10};
    const char* headers[] = {"序号", "班级", "人数"};
    tableStart(3, widths, headers);
    for (int i = 0; i < classCount; ++i)
        tableRow("dsd", i+1, schoolClasses[i].name, schoolClasses[i].count);
    tableEnd();
    if (!classCount) printf("暂无班级，请联系管理员添加。\n");
}
int selectClass(void) {
    listClasses();
    if (!classCount) return -1;
    return safeReadInt("选择班级序号 (0取消): ", 0, classCount)-1;
}
void addClass(void) {
    if (classCount >= MAX_CLASSES) { printf("班级数量已达上限！\n"); return; }
    char name[CLASS_NAME_LEN];
    safeReadString(name, sizeof(name), "班级名称: ");
    for (int i = 0; i < classCount; ++i)
        if (!strcmp(name, schoolClasses[i].name)) { printf("班级已存在！\n"); return; }
    memset(&schoolClasses[classCount], 0, sizeof(SchoolClass));
    strcpy(schoolClasses[classCount++].name, name);
    saveAll(); printf("班级添加成功！\n");
}
void chooseMyClass(void) {
    Student* st = findStudentByUsername(currentUser.username);
    if (!st) return;
    int c = selectClass();
    if (c < 0) return;
    if (schoolClasses[c].count >= MAX_STUDENTS) { printf("班级已满！\n"); return; }
    removeClassMember(st->stuNum);
    strcpy(schoolClasses[c].members[schoolClasses[c].count++], st->stuNum);
    saveAll(); printf("已选择班级: %s\n", schoolClasses[c].name);
}

/* UTF-8 terminal cells: Chinese/full-width characters occupy two columns.
   Clip whole code points with an ellipsis rather than spilling into the next cell. */
static int tableColumns;
static int tableWidths[8];
static unsigned int nextCodePoint(const unsigned char** text) {
    const unsigned char* s = *text;
    unsigned int cp = *s++;
    int n = cp < 0x80 ? 0 : cp >= 0xf0 ? 3 : cp >= 0xe0 ? 2 : cp >= 0xc0 ? 1 : 0;
    if (n) cp &= (1u << (6-n))-1;
    for (int i = 0; i < n; ++i) {
        if ((*s & 0xc0) != 0x80) { *text = s; return '?'; }
        cp = (cp << 6) | (*s++ & 0x3f);
    }
    *text = s; return cp;
}
static int cellWidth(unsigned int cp) {
    if ((cp >= 0x300 && cp <= 0x36f) || (cp >= 0xfe00 && cp <= 0xfe0f) || cp == 0x200d) return 0;
    if (cp < 32 || (cp >= 127 && cp < 160)) return 1;
    return (cp >= 0x1100 && (cp <= 0x115f || cp == 0x2329 || cp == 0x232a ||
        (cp >= 0x2e80 && cp <= 0xa4cf) || (cp >= 0xac00 && cp <= 0xd7a3) ||
        (cp >= 0xf900 && cp <= 0xfaff) || (cp >= 0xfe10 && cp <= 0xfe6f) ||
        (cp >= 0xff01 && cp <= 0xff60) || (cp >= 0xffe0 && cp <= 0xffe6) ||
        (cp >= 0x1f300 && cp <= 0x1faff) || (cp >= 0x20000 && cp <= 0x3fffd))) ? 2 : 1;
}
static void tableCell(const char* text, int width) {
    const unsigned char* s = (const unsigned char*)text;
    int total = 0, used = 0;
    while (*s) total += cellWidth(nextCodePoint(&s));
    s = (const unsigned char*)text;
    int limit = total > width ? width-3 : width;
    while (*s) {
        const unsigned char* start = s;
        unsigned int cp = nextCodePoint(&s);
        int w = cellWidth(cp);
        if (used + w > limit) break;
        if (cp < 32 || (cp >= 127 && cp < 160)) putchar('?');
        else fwrite(start, 1, s-start, stdout);
        used += w;
    }
    if (total > width) { printf("..."); used += 3; }
    while (used++ < width) putchar(' ');
}
void tableEnd(void) {
    putchar('+');
    for (int i = 0; i < tableColumns; ++i) {
        for (int j = 0; j < tableWidths[i]+2; ++j) putchar('-');
        putchar('+');
    }
    putchar('\n');
}
void tableStart(int columns, const int* widths, const char* const* headers) {
    tableColumns = columns;
    for (int i = 0; i < columns; ++i) tableWidths[i] = widths[i];
    tableEnd(); putchar('|');
    for (int i = 0; i < columns; ++i) {
        putchar(' '); tableCell(headers[i], widths[i]); printf(" |");
    }
    putchar('\n'); tableEnd();
}
void tableRow(const char* types, ...) {
    va_list args; va_start(args, types);
    putchar('|');
    for (int i = 0; i < tableColumns; ++i) {
        char buffer[64]; const char* text = buffer;
        if (types[i] == 's') text = va_arg(args, const char*);
        else if (types[i] == 'd') snprintf(buffer, sizeof(buffer), "%d", va_arg(args, int));
        else snprintf(buffer, sizeof(buffer), "%.1f", va_arg(args, double));
        putchar(' '); tableCell(text, tableWidths[i]); printf(" |");
    }
    putchar('\n'); va_end(args);
}
void printStudentCourses(const Student* st) {
    const int widths[] = {6, 24, 10};
    const char* headers[] = {"序号", "科目", "成绩"};
    tableStart(3, widths, headers);
    for (int j = 0; j < st->courseCount; ++j)
        tableRow("dsf", j+1, st->courses[j].course, (double)st->courses[j].score);
    tableEnd();
    if (!st->courseCount) printf("暂无成绩记录。\n");
}
static int canListStudent(const Student* st) {
    if (currentUser.type == ADMIN) return 1;
    if (currentUser.type != TEACHER) return 0;
    for (int j = 0; j < st->teacherCount; ++j)
        if (!strcmp(st->teachers[j], currentUser.username)) return 1;
    return 0;
}
void listStudentsAndScores(int scores) {
    const int widths[] = {6, 20, 16, 24, 20, 10};
    const char* studentHeaders[] = {"序号", "学号", "姓名", "班级", "科目数", "平均分"};
    const char* scoreHeaders[] = {"序号", "学号", "姓名", "班级", "科目", "成绩"};
    tableStart(6, widths, scores ? scoreHeaders : studentHeaders);
    int count = 0;
    for (int i = 0; i < studentCount; ++i) {
        Student* st = &students[i];
        if (!canListStudent(st)) continue;
        int c = studentClass(st->stuNum);
        const char* name = c < 0 ? "未选择" : schoolClasses[c].name;
        if (!scores) {
            char average[32];
            if (st->courseCount) snprintf(average, sizeof(average), "%.1f", st->average);
            else strcpy(average, "--");
            tableRow("dsssds", ++count, st->stuNum, st->name, name, st->courseCount, average);
        } else if (!st->courseCount) {
            tableRow("dsssss", ++count, st->stuNum, st->name, name, "暂无成绩", "--");
        } else for (int j = 0; j < st->courseCount; ++j) {
            tableRow("dssssf", ++count, st->stuNum, st->name, name, st->courses[j].course, (double)st->courses[j].score);
        }
    }
    tableEnd();
    if (!count) printf("暂无可查看的记录。\n");
}
