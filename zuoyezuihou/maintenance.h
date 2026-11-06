#ifndef MAINTENANCE_H
#define MAINTENANCE_H
#include "system.h"

/* Sidecar metadata keeps the old in-memory record layouts readable. */
extern char userDisplayNames[MAX_USERS][60];
extern int classInactive[MAX_CLASSES];
typedef enum
{
    RESULT_OK,
    RESULT_DENIED,
    RESULT_INVALID,
    RESULT_IO
} OperationResult;
int storageOpen(void);
int storageLoad(void); /* 0: legacy directory, 1: versioned dataset */
int storageCommit(void);
void storageBaseline(void);
void storageBackupLegacy(void);
void auditEvent(const char *action, const char *detail);
void auditLogin(const char *username, int success);
void showAudit(void);
OperationResult assignClassMember(const char *id, int classIndex);
void classManagement(void);
void editRecords(void);
void searchRecords(void);
void filteredList(int scores);
int hasTeacherAccess(const Student *st);
int requireTeacherStudents(void);
int teacherStudentCount(void);
int courseIndex(const Student *st, const char *course);
void classStatistics(void);
void classRanking(void);
void classQuery(void);
void csvImport(void);
void csvExport(void);
const char *displayName(const char *login);
int pageNext(int count);
extern int demoSeedVersion;
int inputFinished(void);
InputStatus readPassword(char *buffer, size_t size, const char *prompt);
int validStudentId(const char *text);
int validTeacherId(const char *text);
int validPersonName(const char *text);
InputStatus readStudentId(char *text, size_t size, const char *prompt);
InputStatus readTeacherId(char *text, size_t size, const char *prompt);
InputStatus readPersonName(char *text, size_t size, const char *prompt);
int confirmAction(const char *prompt);
typedef struct
{
    const char *label;
    void (*action)(void);
} MenuItem;
int menuChoice(const char *title, const char *const *items, int count);
void runMenu(const char *path, const MenuItem *items, int count);
int selectTeachers(char chosen[MAX_TEACHERS_PER_STUDENT][20]);
OperationResult createTeacher(const User *teacher, const char *name);
OperationResult createStudent(const Student *student, int classIndex);
void editStudent(void);
void editTeacher(void);
void editTeaching(void);
void teacherList(void);
void studentOverview(void);
void teacherOverview(void);
void classOverview(void);
void systemHome(void);
int selectCourse(char *course, size_t capacity);
void adminScoreQuery(void);
int migrateTeachers(void);
int seedDemoData(int confirmed);
void seedDemoMenu(void);
#endif
