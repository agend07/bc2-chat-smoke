/* Standalone (msimg32.dll) identity of the chat / smoke modules. build.ps1 force-includes this header (/FI) into
 * every translation unit, so client_chat.c, client_smoke.c and client_chat_selftest.c stay byte-identical with the
 * copies in the BC2 Local Replay proxy, where the defaults are BC2Replay / "[dinput8]" / "dinput8 proxy". */
#ifndef BC2_STANDALONE_H
#define BC2_STANDALONE_H

#define BC2_CHAT_STANDALONE  1                 /* no /rec command, no recorder header */
#define BC2_CHAT_APPDIR      "BC2ChatSmoke"    /* %LOCALAPPDATA%\BC2ChatSmoke: lists, config.ini, logs\ */
#define BC2_CHAT_TAG         "[chat]"          /* sender of the local replies */
#define BC2_CHAT_LOG_PREFIX  "bc2-chat-smoke"  /* OutputDebugString prefix and the list-file header */

#endif
