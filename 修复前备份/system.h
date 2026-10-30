#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USERS 100
#define MAX_STUDENTS 100
#define MAX_COURSES 10
#define MAX_TEACHERS_PER_STUDENT 10  // 每个学生最多分配10位教师
#define STU_NUM_LEN 20
#define PASS_SCORE 60.0f
#define LOGIN_ATTEMPTS 3

typedef enum { ADMIN, TEACHER, STUDENT } UserType;

typedef struct {
    char course[30];
    float score;
} Course;

typedef struct {
    char username[20];
    char password[20];
    UserType type;
    char teachCourse[30];
} User;

typedef struct {
    char stuNum[STU_NUM_LEN];
    char name[20];
    Course courses[MAX_COURSES];
    char teachers[MAX_TEACHERS_PER_STUDENT][20];  // 支持多位教师
    int teacherCount;                             // 教师数量
    int courseCount;
    float average;
} Student;

extern User users[MAX_USERS];
extern int userCount;
extern Student students[MAX_USERS];
extern int studentCount;
extern User currentUser;

void initSystem();
void loadAll();
void saveAll();
int login(UserType requiredType);
void safeReadString(char* buffer, int size, const char* prompt);
int safeReadInt(const char* prompt, int min, int max);
float safeReadFloat(const char* prompt, float min, float max);
int userExists(const char* username);
int findStudentIndex(const char* stuNum);
Student* findStudentByUsername(const char* username);

#endif
