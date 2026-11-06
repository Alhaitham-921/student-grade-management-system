#include "maintenance.h"
#include <windows.h>
#include <io.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>
static int ended;
int inputFinished(void)
{
    return ended;
}
static InputStatus readLine(char *buffer, size_t size, const char *prompt)
{
    buffer[0] = 0;
    if (ended)
        return INPUT_END;
    for (;;)
    {
        printf("%s", prompt);
        fflush(stdout);
        size_t n = 0;
        int ch, over = 0, control = 0;
        while ((ch = getchar()) != EOF && ch != '\n')
        {
            if (ch == '\r')
                continue;
            if (ch == 0 || (ch < 32 && ch != '\t') || ch == 127)
                control = 1;
            if (n + 1 < size)
                buffer[n++] = (char)ch;
            else
                over = 1;
        }
        buffer[n] = 0;
        if (ch == EOF)
        {
            ended = 1;
            if (ferror(stdin))
                printf("输入读取结束。\n");
            return INPUT_END;
        }
        if (over)
        {
            printf("输入过长，请重新输入！\n");
            continue;
        }
        if (control)
        {
            printf("输入含有非法控制字符，请重新输入。\n");
            continue;
        }
        if (!strcmp(buffer, ":q"))
        {
            printf("已取消操作。\n");
            return INPUT_CANCEL;
        }
        return INPUT_OK;
    }
}
InputStatus safeReadString(char *buffer, size_t size, const char *prompt)
{
    for (;;)
    {
        InputStatus result = readLine(buffer, size, prompt);
        if (result != INPUT_OK)
            return result;
        if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, buffer, -1, NULL, 0))
        {
            printf("请输入有效UTF-8文字。\n");
            continue;
        }
        int nonempty = 0;
        for (size_t i = 0; buffer[i]; i++)
            if (!isspace((unsigned char)buffer[i]))
                nonempty = 1;
        if (nonempty)
            return INPUT_OK;
        printf("输入不能为空！\n");
    }
}
int safeReadInt(const char *prompt, int min, int max)
{
    char text[100];
    for (;;)
    {
        if (readLine(text, sizeof(text), prompt) != INPUT_OK)
            return -1;
        /* Console users often paste a choice with surrounding whitespace. */
        char *first = text;
        while (*first == ' ' || *first == '\t')
            first++;
        memmove(text, first, strlen(first) + 1);
        size_t length = strlen(text);
        while (length && (text[length - 1] == ' ' || text[length - 1] == '\t'))
            text[--length] = 0;
        int digits = text[0] != 0;
        for (size_t i = 0; text[i]; i++)
            if (text[i] < '0' || text[i] > '9')
                digits = 0;
        errno = 0;
        char *end;
        long n = strtol(text, &end, 10);
        if (digits && !errno && !*end && n >= min && n <= max)
            return (int)n;
        printf("请输入 %d 到 %d 之间的整数。\n", min, max);
    }
}
float safeReadFloat(const char *prompt, float min, float max)
{
    char text[100];
    for (;;)
    {
        if (readLine(text, sizeof(text), prompt) != INPUT_OK)
            return NAN;
        errno = 0;
        char *end;
        float n = strtof(text, &end);
        int parsed = end != text;
        while (isspace((unsigned char)*end))
            end++;
        if (parsed && !*end && !errno && isfinite(n) && n >= min && n <= max)
            return n;
        printf("请输入 %.1f 到 %.1f 之间的分数。\n", min, max);
    }
}
int validStudentId(const char *s)
{
    size_t n = strlen(s);
    if (!n || n >= STU_NUM_LEN)
        return 0;
    for (size_t i = 0; i < n; i++)
        if (s[i] < '0' || s[i] > '9')
            return 0;
    return 1;
}
int validTeacherId(const char *s)
{
    if (strlen(s) != 5 || s[0] != 'T')
        return 0;
    for (int i = 1; i < 5; i++)
        if (s[i] < '0' || s[i] > '9')
            return 0;
    return strcmp(s, "T0000") != 0;
}
int validPersonName(const char *text)
{
    wchar_t wide[128];
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, wide, 128))
        return 0;
    int letters = 0;
    for (int i = 0; wide[i]; i++)
    {
        uint32_t cp = wide[i];
        if (cp >= 0xd800 && cp <= 0xdbff)
        {
            if (wide[i + 1] < 0xdc00 || wide[i + 1] > 0xdfff)
                return 0;
            cp = 0x10000 + ((cp - 0xd800) << 10) + (wide[++i] - 0xdc00);
        }
        if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z') || (cp >= 0x3400 && cp <= 0x4dbf) ||
            (cp >= 0x4e00 && cp <= 0x9fff) || (cp >= 0x20000 && cp <= 0x323af))
            letters++;
        else if (cp != ' ' && cp != 0xb7 && cp != '-' && cp != '\'')
            return 0;
    }
    return letters > 0;
}
static InputStatus readValidated(char *text, size_t capacity, const char *prompt, int (*valid)(const char *),
                                 const char *error, int trim)
{
    for (;;)
    {
        InputStatus result = safeReadString(text, capacity, prompt);
        if (result != INPUT_OK)
            return result;
        if (trim)
        {
            char *first = text;
            while (*first == ' ')
                first++;
            memmove(text, first, strlen(first) + 1);
            size_t n = strlen(text);
            while (n && text[n - 1] == ' ')
                text[--n] = 0;
        }
        if (valid(text))
            return INPUT_OK;
        printf("%s\n", error);
    }
}
InputStatus readStudentId(char *s, size_t n, const char *p)
{
    return readValidated(s, n, p, validStudentId, "学号必须为1到19位数字字符串，不可为空。", 0);
}
InputStatus readTeacherId(char *s, size_t n, const char *p)
{
    return readValidated(s, n, p, validTeacherId, "工号必须为T0001至T9999。", 0);
}
InputStatus readPersonName(char *s, size_t n, const char *p)
{
    return readValidated(s, n, p, validPersonName, "姓名必须包含汉字或英文字母，不能包含数字或非法符号。", 1);
}
int confirmAction(const char *prompt)
{
    return safeReadInt(prompt, 0, 1) == 1;
}
int menuChoice(const char *title, const char *const *items, int count)
{
    if (ended)
        return -1;
    printf("\n========== %s ==========\n", title);
    for (int i = 0; i < count; i++)
        printf("%d. %s\n", i + 1, items[i]);
    return safeReadInt("请选择功能: ", 1, count);
}
void runMenu(const char *path, const MenuItem *items, int count)
{
    while (!ended)
    {
        clearScreen();
        const char *labels[20];
        if (count > 20)
            return;
        for (int i = 0; i < count; i++)
            labels[i] = items[i].label;
        printf("当前用户: %s\n", currentUser.username);
        if (currentUser.type == TEACHER)
            printf("教师姓名: %s | 任教科目: %s | 授权学生: %d人\n", displayName(currentUser.username),
                   currentUser.teachCourse, teacherStudentCount());
        int choice = menuChoice(path, labels, count);
        if (choice < 1 || choice == count)
            return;
        if (items[choice - 1].action)
            items[choice - 1].action();
        pauseScreen();
    }
}
void clearScreen(void)
{
    if (_isatty(_fileno(stdin)))
        system("cls");
}
void pauseScreen(void)
{
    if (!ended && _isatty(_fileno(stdin)))
    {
        char text[8];
        readLine(text, sizeof(text), "按回车继续...");
    }
}

InputStatus readPassword(char *buffer, size_t size, const char *prompt)
{
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    if (!GetConsoleMode(input, &mode))
        return safeReadString(buffer, size, prompt);
    if (!SetConsoleMode(input, mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT)))
    {
        printf("无法启用密码掩码，已取消输入。\n");
        return INPUT_CANCEL;
    }
    InputStatus result = INPUT_CANCEL;
    wchar_t text[256] = {0};
    size_t length = 0;
    buffer[0] = 0;
    printf("%s", prompt);
    fflush(stdout);
    for (;;)
    {
        INPUT_RECORD record;
        DWORD n = 0;
        if (!ReadConsoleInputW(input, &record, 1, &n) || !n)
        {
            ended = 1;
            result = INPUT_END;
            break;
        }
        if (record.EventType != KEY_EVENT || !record.Event.KeyEvent.bKeyDown)
            continue;
        KEY_EVENT_RECORD key = record.Event.KeyEvent;
        wchar_t ch = key.uChar.UnicodeChar;
        if (key.wVirtualKeyCode == VK_ESCAPE || ch == 3)
        {
            printf("\n已取消操作。");
            break;
        }
        if (ch == 26)
        {
            ended = 1;
            result = INPUT_END;
            break;
        }
        if (key.wVirtualKeyCode == VK_BACK)
        {
            for (WORD repeat = 0; repeat < key.wRepeatCount && length; repeat++)
            {
                length--;
                if (length && text[length] >= 0xdc00 && text[length] <= 0xdfff && text[length - 1] >= 0xd800 &&
                    text[length - 1] <= 0xdbff)
                    length--;
                text[length] = 0;
                printf("\b \b");
            }
            fflush(stdout);
            continue;
        }
        if (key.wVirtualKeyCode == VK_RETURN)
        {
            text[length] = 0;
            if (!wcscmp(text, L":q"))
            {
                printf("\n已取消操作。");
                break;
            }
            int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, -1, NULL, 0, NULL, NULL);
            int nonempty = 0;
            for (size_t i = 0; i < length; i++)
                if (text[i] != L' ' && text[i] != L'\t')
                    nonempty = 1;
            if (nonempty && bytes > 0 && (size_t)bytes <= size)
            {
                WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, -1, buffer, (int)size, NULL, NULL);
                result = INPUT_OK;
                break;
            }
            printf("\n密码不能为空、编码无效或超过%zu字节，请重新输入。\n%s", size - 1, prompt);
            length = 0;
            text[0] = 0;
            fflush(stdout);
            continue;
        }
        if ((ch < 32 && ch != L'\t') || ch == 127)
            continue;
        for (WORD repeat = 0; repeat < key.wRepeatCount; repeat++)
        {
            if (length + 1 >= sizeof(text) / sizeof(text[0]))
                continue;
            text[length++] = ch;
            text[length] = 0;
            if (!(ch >= 0xdc00 && ch <= 0xdfff))
                putchar('*');
        }
        fflush(stdout);
    }
    SecureZeroMemory(text, sizeof(text));
    if (!SetConsoleMode(input, mode))
    {
        fprintf(stderr, "恢复终端输入模式失败，请重新打开程序。\n");
        buffer[0] = 0;
        ended = 1;
        return INPUT_END;
    }
    putchar('\n');
    return result;
}
