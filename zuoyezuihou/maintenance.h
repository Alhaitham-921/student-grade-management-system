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
int courseIndex(const Student *st, const char *course);
void classStatistics(void);
void classRanking(void);
void classQuery(void);
void csvImport(void);
void csvExport(void);
const char *displayName(const char *login);
int pageNext(int count);
#endif
