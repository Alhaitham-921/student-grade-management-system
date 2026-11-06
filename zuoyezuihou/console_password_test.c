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
static void textKeys(const wchar_t *text)
{
    for (; *text; text++)
        key(*text == L'\r' ? VK_RETURN : 0, *text);
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
    textKeys(L":q\r");
    if (readPassword(password, sizeof(password), "Password: ") != INPUT_CANCEL ||
        !GetConsoleMode(inputHandle, &after) || after != original)
    {
        fprintf(stderr, "Console typed cancellation failed\n");
        return 1;
    }
    key('C', 3);
    if (readPassword(password, sizeof(password), "Password: ") != INPUT_CANCEL ||
        !GetConsoleMode(inputHandle, &after) || after != original)
    {
        fprintf(stderr, "Console Ctrl+C cancellation failed\n");
        return 1;
    }
    textKeys(L"aaaaaaaaaaaaaaaaaaaa\rbbbbbb\r");
    if (readPassword(password, sizeof(password), "Password: ") != INPUT_OK || strcmp(password, "bbbbbb") ||
        !GetConsoleMode(inputHandle, &after) || after != original)
    {
        fprintf(stderr, "Console overlong-password retry failed\n");
        return 1;
    }
    if (freopen_s(&console, "CONIN$", "r", stdin))
    {
        fprintf(stderr, "Console standard-input binding failed\n");
        return 1;
    }
    /* freopen replaces the OS standard handle with its CRT-owned read-only
       handle. Preserve the read/write handle used by the real password reader. */
    if (!SetStdHandle(STD_INPUT_HANDLE, inputHandle))
    {
        fprintf(stderr, "Console standard-handle restoration failed\n");
        return 1;
    }
    /* Exercise the actual terminal branch; redirected Python tests skip pageNext. */
    textKeys(L"0\r1\r");
    if (!pageNext(10))
    {
        fprintf(stderr, "Console pagination invalid-input/continue failed\n");
        return 1;
    }
    textKeys(L"2\r");
    if (pageNext(20))
    {
        fprintf(stderr, "Console pagination return failed\n");
        return 1;
    }
    textKeys(L"4\r");
    if (safeReadInt("Choice: ", 1, 4) != 4)
    {
        fprintf(stderr, "Console line input after masked password failed\n");
        return 1;
    }
    textKeys(L"abcdef\r");
    result = readPassword(password, sizeof(password), "Password: ");
    if (result != INPUT_OK || strcmp(password, "abcdef"))
    {
        fprintf(stderr, "Console masked password after line input failed result=%d length=%zu keyError=%ld modeOK=%d win32=%lu\n",
                result, strlen(password), failed, GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &after), GetLastError());
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
                    "restores console mode; typed/Ctrl+C cancellation, overlong retry, real pagination and "
                    "line/password input transitions\n");
    return 0;
}
#endif
