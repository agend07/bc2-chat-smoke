/* bc2-chat-smoke: msimg32.dll proxy loader for Battlefield: Bad Company 2 (retail / Project Rome and Steam
 * BFBC2Game.exe).
 *
 * The exe imports msimg32.dll and Windows resolves it from the exe's folder first, so this file is loaded at
 * startup. It forwards the five system exports to the real %SystemRoot%\System32\msimg32.dll (naked jmp stubs,
 * x86 only; see msimg32.def) and, inside BFBC2Game.exe only, starts the chat module (client_chat.c: instance-vtable
 * clone hooks + a 250 ms monitor thread that also drives the smoke recolour, client_smoke.c). Any other process
 * that happens to load this dll gets the plain forwarders and nothing else.
 *
 * The chat / smoke modules are byte-identical copies of the same files in the BC2 Local Replay proxy (rome_proxy);
 * the standalone identity (app folder, reply tag, log prefix) comes from standalone.h, force-included by build.ps1.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "standalone.h"
#include "client_chat.h"
#include "version.h"

/* =========================================================================
 * System msimg32.dll forwarding
 * ========================================================================= */
static HMODULE g_real_msimg32 = NULL;
static FARPROC g_pAlphaBlend = NULL;
static FARPROC g_pDllInitialize = NULL;
static FARPROC g_pGradientFill = NULL;
static FARPROC g_pTransparentBlt = NULL;
static FARPROC g_pVSetDdrawflag = NULL;

static void LoadRealMsimg32(void) {
    if (g_real_msimg32) return;
    char sys_dir[MAX_PATH];
    if (GetSystemDirectoryA(sys_dir, MAX_PATH)) {
        char full_path[MAX_PATH];
        _snprintf_s(full_path, sizeof(full_path), _TRUNCATE, "%s\\msimg32.dll", sys_dir);
        g_real_msimg32 = LoadLibraryA(full_path);
        if (g_real_msimg32) {
            g_pAlphaBlend = GetProcAddress(g_real_msimg32, "AlphaBlend");
            g_pDllInitialize = GetProcAddress(g_real_msimg32, "DllInitialize");
            g_pGradientFill = GetProcAddress(g_real_msimg32, "GradientFill");
            g_pTransparentBlt = GetProcAddress(g_real_msimg32, "TransparentBlt");
            g_pVSetDdrawflag = GetProcAddress(g_real_msimg32, "vSetDdrawflag");
        }
    }
}

__declspec(naked) void Proxy_AlphaBlend(void) {
    __asm { jmp dword ptr [g_pAlphaBlend] }
}
__declspec(naked) void Proxy_DllInitialize(void) {
    __asm { jmp dword ptr [g_pDllInitialize] }
}
__declspec(naked) void Proxy_GradientFill(void) {
    __asm { jmp dword ptr [g_pGradientFill] }
}
__declspec(naked) void Proxy_TransparentBlt(void) {
    __asm { jmp dword ptr [g_pTransparentBlt] }
}
__declspec(naked) void Proxy_vSetDdrawflag(void) {
    __asm { jmp dword ptr [g_pVSetDdrawflag] }
}

/* =========================================================================
 * Attach gate + first-run migration
 * ========================================================================= */
static const char CLIENT_NAME[] = "bfbc2game.exe";

static void log_line(const char *text)
{
    char line[MAX_PATH + 128];
    wsprintfA(line, BC2_CHAT_LOG_PREFIX ": %s\r\n", text);
    OutputDebugStringA(line);
}

/* 1 only inside BFBC2Game.exe (lower-cased exe base name), the same gate as the shared proxy's bc2_mode_attach */
static int is_game_client(void)
{
    char exe[MAX_PATH], name[MAX_PATH];
    const char *base;
    DWORD len = GetModuleFileNameA(NULL, exe, sizeof(exe));
    if (len == 0 || len >= sizeof(exe))
        return 0;
    base = strrchr(exe, '\\');
    base = base ? base + 1 : exe;
    lstrcpynA(name, base, sizeof(name));
    CharLowerA(name);
    return lstrcmpA(name, CLIENT_NAME) == 0;
}

/* The earlier bc2_mute msimg32.dll kept muted_players.txt next to the dll (game folder); this build keeps it in
 * %LOCALAPPDATA%\BC2ChatSmoke. On the first run copy the old list once (never overwrite). Must run before
 * bc2_client_chat_attach(): its monitor thread creates an empty list file when none exists. */
static void migrate_old_mute_list(HMODULE self)
{
    static const char LIST[] = "muted_players.txt";
    char src[MAX_PATH], dst[MAX_PATH], line[MAX_PATH + 64];
    char *slash;
    DWORD n = GetModuleFileNameA(self, src, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        return;
    slash = strrchr(src, '\\');
    if (!slash || (size_t)(slash + 1 - src) + sizeof(LIST) > MAX_PATH)
        return;
    lstrcpyA(slash + 1, LIST);
    if (GetFileAttributesA(src) == INVALID_FILE_ATTRIBUTES)
        return;
    n = GetEnvironmentVariableA("LOCALAPPDATA", dst, MAX_PATH);
    if (n == 0 || n + sizeof(BC2_CHAT_APPDIR) + sizeof(LIST) + 2 >= MAX_PATH)
        return;
    lstrcatA(dst, "\\" BC2_CHAT_APPDIR);
    CreateDirectoryA(dst, NULL);
    lstrcatA(dst, "\\");
    lstrcatA(dst, LIST);
    if (CopyFileA(src, dst, TRUE)) {   /* TRUE = fail if the new list already exists */
        wsprintfA(line, "migrated the old mute list from the game folder to %s", dst);
        log_line(line);
    }
}

BOOL WINAPI DllMain(HINSTANCE self, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(self);
        LoadRealMsimg32();
        if (is_game_client()) {
            log_line("bc2-chat-smoke " BC2_VERSION_STRING " in BFBC2Game.exe");
            migrate_old_mute_list(self);
            bc2_client_chat_attach();   /* starts the monitor thread; hooks only on a known exe (bc2_exe.h) */
        }
    } else if (reason == DLL_PROCESS_DETACH) {
        bc2_client_chat_detach();       /* stop flag only, never wait */
    }
    return TRUE;
}
