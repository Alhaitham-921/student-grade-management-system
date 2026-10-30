#include "system.h"

User users[MAX_USERS];
int userCount = 0;
Student students[MAX_STUDENTS];
int studentCount = 0;
User currentUser;

// 安全读取字符串
void safeReadString(char* buffer, int size, const char* prompt) {
    printf(prompt);
    fgets(buffer, size, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
}

// 安全读取整数
int safeReadInt(const char* prompt, int min, int max) {
    int value;
    char buffer[100];

    while (1) {
        printf(prompt);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            continue;
        }

        if (sscanf(buffer, "%d", &value) == 1) {
            if (value >= min && value <= max) {
                return value;
            }
        }

        printf("输入错误，请输入 %d 到 %d 之间的整数。\n", min, max);
    }
}

// 安全读取浮点数
float safeReadFloat(const char* prompt, float min, float max) {
    float value;
    char buffer[100];

    while (1) {
        printf(prompt);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            continue;
        }

        if (sscanf(buffer, "%f", &value) == 1) {
            if (value >= min && value <= max) {
                return value;
            }
        }

        printf("输入错误，请输入 %.1f 到 %.1f 之间的浮点数。\n", min, max);
    }
}

// 初始化系统，创建默认管理员账户
void initSystem() {
    if (userCount == 0) {
        User admin = { 0 };
        strcpy(admin.username, "admin");
        strcpy(admin.password, "admin");
        admin.type = ADMIN;
        users[userCount++] = admin;
        saveAll();
        printf("初始管理员账户已创建（用户名: admin, 密码: admin）\n");
        printf("请登录后修改默认密码！\n");
    }
}

// 从文件加载数据
void loadAll() {
    FILE* file = fopen("users.dat", "rb");
    if (file) {
        fread(&userCount, sizeof(int), 1, file);
        fread(users, sizeof(User), userCount, file);
        fclose(file);
    }

    file = fopen("students.dat", "rb");
    if (file) {
        fread(&studentCount, sizeof(int), 1, file);
        fread(students, sizeof(Student), studentCount, file);
        fclose(file);
    }
}

// 保存数据到文件
void saveAll() {
    FILE* file = fopen("users.dat", "wb");
    if (file) {
        fwrite(&userCount, sizeof(int), 1, file);
        fwrite(users, sizeof(User), userCount, file);
        fclose(file);
    }

    file = fopen("students.dat", "wb");
    if (file) {
        fwrite(&studentCount, sizeof(int), 1, file);
        fwrite(students, sizeof(Student), studentCount, file);
        fclose(file);
    }
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
        if (strcmp(students[i].name, username) == 0) {
            return &students[i];
        }
    }
    return NULL;
}

// 登录函数
int login(UserType requiredType) {
    char username[20];
    char password[20];
    int attempts = 3;

    while (attempts > 0) {
        printf("\n[登录 - %s用户]\n",
            requiredType == ADMIN ? "管理员" :
            requiredType == TEACHER ? "教师" : "学生");
        safeReadString(username, sizeof(username), "用户名: ");
        safeReadString(password, sizeof(password), "密码: ");

        for (int i = 0; i < userCount; i++) {
            if (strcmp(users[i].username, username) == 0 &&
                strcmp(users[i].password, password) == 0 &&
                users[i].type == requiredType) {
                currentUser = users[i];
                return 1;
            }
        }

        attempts--;
        printf("用户名、密码或用户类型错误，你还有 %d 次尝试机会。\n", attempts);
    }

    printf("登录失败！\n");
    return 0;
}