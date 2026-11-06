/* Runs only in the test executable, on a private console. No user console or data files are touched. */
#ifdef SGMS_TESTING
#include "maintenance.h"
#include <windows.h>
#include <process.h>
#include <wchar.h>
static HANDLE inputHandle, outputHandle;
static volatile LONG failed;
static void key(WORD vk, wchar_t ch)
{
    INPUT_RECORD r = {0};
    DWORD written;
    r.EventType = KEY_EVENT;
    r.Event.KeyEvent.bKeyDown = TRUE;
    r.Event.KeyEvent.wRepeatCount = 1;
    r.Event.KeyEvent.wVirtualKeyCode = vk;
    r.Event.KeyEvent.uChar.UnicodeChar = ch;
    if (!WriteConsoleInputW(inputHandle, &r, 1, &written) || written != 1)
        InterlockedExchange(&failed, 1);
}
static int starsAtCursor(int stars)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(outputHandle, &info))
        return 0;
    WCHAR row[1024] = {0};
    DWORD n = 0;
    COORD p = {0, info.dwCursorPosition.Y};
    if (!ReadConsoleOutputCharacterW(outputHandle, row, 1023, p, &n))
        return 0;
    int count = 0;
    for (DWORD i = 10; i < n; i++)
    {
        if (row[i] == '*')
            count++;
        if (row[i] == 'a' || row[i] == 'b' || row[i] == 'c' || row[i] == 'd')
            return 0;
    }
    return count == stars;
}
static int waitStars(int n)
{
    for (int i = 0; i < 200; i++)
    {
        if (starsAtCursor(n))
            return 1;
        Sleep(10);
    }
    return 0;
}
static unsigned __stdcall feed(void *ignored)
{
    (void)ignored;
    key('A', L'a');
    key('B', L'b');
    key('C', L'c');
    if (!waitStars(3))
        InterlockedExchange(&failed, 1);
    key(VK_BACK, L'\b');
    if (!waitStars(2))
        InterlockedExchange(&failed, 1);
    key('D', L'd');
    if (!waitStars(3))
        InterlockedExchange(&failed, 1);
    key(VK_RETURN, L'\r');
    return 0;
}
int verifyConsolePassword(void)
{
    FreeConsole();
    if (!AllocConsole())
    {
        fprintf(stderr, "Console test: AllocConsole failed %lu\n", GetLastError());
        return 1;
    }
    HWND own = GetConsoleWindow();
    if (own)
        ShowWindow(own, SW_HIDE);
    inputHandle = CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                              OPEN_EXISTING, 0, NULL);
    outputHandle = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_EXISTING, 0, NULL);
    if (inputHandle == INVALID_HANDLE_VALUE || outputHandle == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Console handle setup failed %lu\n", GetLastError());
        return 1;
    }
    SetStdHandle(STD_INPUT_HANDLE, inputHandle);
    FILE *console;
    freopen_s(&console, "CONOUT$", "w", stdout);
    DWORD original, after;
    if (!GetConsoleMode(inputHandle, &original))
    {
        fprintf(stderr, "Console mode query failed %lu\n", GetLastError());
        return 1;
    }
    HANDLE worker = (HANDLE)_beginthreadex(NULL, 0, feed, NULL, 0, NULL);
    if (!worker)
        return 1;
    char password[20];
    InputStatus result = readPassword(password, sizeof(password), "Password: ");
    WaitForSingleObject(worker, 5000);
    CloseHandle(worker);
    if (result != INPUT_OK || strcmp(password, "abd") || failed || !GetConsoleMode(inputHandle, &after) ||
        original != after)
    {
        fprintf(stderr, "Console mask check failed result=%d worker=%ld modeBefore=%lu modeAfter=%lu error=%lu\n",
                result, failed, original, after, GetLastError());
        return 1;
    }
    key(VK_ESCAPE, 27);
    if (readPassword(password, sizeof(password), "Password: ") != INPUT_CANCEL ||
        !GetConsoleMode(inputHandle, &after) || after != original)
    {
        fprintf(stderr, "Console cancel/EOF restoration check failed %lu\n", GetLastError());
        return 1;
    }
    key('Z', 26);
    if (readPassword(password, sizeof(password), "Password: ") != INPUT_END || !GetConsoleMode(inputHandle, &after) ||
        after != original)
    {
        fprintf(stderr, "Console cancel/EOF restoration check failed %lu\n", GetLastError());
        return 1;
    }
    CloseHandle(inputHandle);
    CloseHandle(outputHandle);
    FreeConsole();
    fprintf(stderr, "PASS: real Win32 console shows stars, hides characters, supports backspace, Escape/EOF and "
                    "restores console mode\n");
    return 0;
}
#endif
