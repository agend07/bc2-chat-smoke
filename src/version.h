/* Release version of bc2-chat-smoke. client_chat.c logs it as the first line of every chat_<pid>.log; build.ps1
 * reads the string for the zip name and, on a tagged CI build, checks it against the tag (v<version>). */
#ifndef BC2_VERSION_H
#define BC2_VERSION_H

#define BC2_VERSION_STRING "0.1.1"

#endif
