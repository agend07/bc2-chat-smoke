# bc2-chat-smoke

In-game chat commands for **Battlefield: Bad Company 2** (retail / Project Rome and Steam `BFBC2Game.exe`), packed into one
standalone `msimg32.dll`:

- `/mute`, `/unmute`, `/mutelist`: hide chat from players or keywords you do not want to read.
- `/hash`, `/unhash`, `/hashlist`: keep a sender visible, but turn their text into symbols.
- `/smoke ...`: recolour smoke-grenade smoke on your screen (static colours, per-grenade palettes, rainbow, pulse,
  Halloween presets).
- `/help`: the command list.

Everything runs on your side only. Replies are shown to you alone, nothing is sent to the server, and other players
see the normal grey smoke. Nothing on disk is modified: not the game exe, not Project Rome's `dinput8.dll`.

Current version: **0.1.2**. Licence: MIT (see `LICENSE`).

## Install

1. Download `bc2-chat-smoke-<version>.zip` from the [Releases](../../releases) page (and `SHA256.txt` if you want to
   verify it, see [Release verification](#release-verification)).
2. Extract `msimg32.dll` into the game folder, next to `BFBC2Game.exe`.
3. Start the game. Open the chat (`J` all, `K` team, `L` squad), type `/help` and press Enter.

The dll is loaded when the game starts, so restart the game after installing or replacing it.

**Uninstall:** delete `msimg32.dll` from the game folder. Your lists and settings stay in
`%LOCALAPPDATA%\BC2ChatSmoke\` until you delete that folder too.

The earlier `bc2_mute` chat filter used the same file name `msimg32.dll`; this project supersedes it, so the new file
simply replaces the old one. Your old mute list is picked up automatically (see [Where the files live](#where-the-files-live)).

## Do not combine with the BC2 Local Replay package

**Remove this `msimg32.dll` before using the BC2 Local Replay package.** Its `dinput8.dll` already contains these
same chat and smoke commands. Both DLLs hook the same game objects (they swap the same vtables), and running them
together is not supported. Use one or the other, never both.

## Commands

Type them in any chat channel. A command is consumed locally and never reaches the server; the reply appears in the
chat window with the sender `[chat]`. Unknown `/text` is sent to the server exactly as typed.

| Command | Effect |
|---|---|
| `/mute <name or word>` | Hide chat lines whose sender name or first word contains the text (case-insensitive substring). Saved to `muted_players.txt`. |
| `/unmute <name or word>` | Remove matching entries from the mute list. |
| `/mutelist` (alias `/muted`) | Show the mute list. |
| `/hash <name>` | A hashed sender stays visible, but their text is rewritten: ASCII letters and digits become `#$%&@!*?`, same length; spaces, punctuation and non-ASCII characters are kept. Saved to `hashed_players.txt`. |
| `/unhash <name>` | Remove matching entries from the hash list. |
| `/hashlist` (alias `/hashed`) | Show the hash list. |
| `/smoke ...` | Smoke colours, see below. |
| `/help` | One-line list of the commands. |

Both lists hold up to 256 entries, are reloaded within 2 seconds after any change, and can be edited by hand: one
entry per line, lines starting with `#` or `;` are comments.

## Smoke colours

`/smoke` recolours all smoke-grenade smoke you see: the vapor cloud, the trail and the projectile trail, for the
launcher and the hand grenade alike. The setting is remembered in `config.ini` and re-applied on the next start and
after every level load. On the very first start (no `[smoke]` setting yet) the Halloween preset is active.

| Command | Effect |
|---|---|
| `/smoke <name>` or `/smoke #rrggbb` | One static colour for all smoke. |
| `/smoke per [N]` | Every grenade gets its own colour from an N-hue palette (2..12, default 8). See the note below. |
| `/smoke rainbow [s]` | Hue cycles through the whole cloud; period in seconds, default 6. |
| `/smoke psycho [s]` | Young and old particles get opposite hues, rotating; default 8 s. |
| `/smoke pulse <color> [s]` | One colour, brightness breathing; default 2 s. |
| `/smoke duo <young> <old>` | Two fixed colours: fresh particles get the first, old particles fade into the second. No animation. |
| `/smoke halloween` | Preset: `duo pumpkin blood`, orange-red smoke darkening into blood red. **Active by default on the first start**; `/smoke off` or any other colour replaces it for good. |
| `/smoke off` | Restore the original greys. |
| `/smoke status` | How many colour objects were found and patched, scan timing, current mode. |
| `/smoke alpha <0..1\|orig>` | Advanced: template tunable (also `ambient`, `vpl`, `normal`). `orig` restores the original value. |

Colour names:

| Group | Names |
|---|---|
| Basic | `red` `green` `blue` `yellow` `cyan` `magenta` `orange` `purple` `pink` `white` `black` `lime` `teal` `gold` `grey` / `gray` |
| Halloween presets | `pumpkin` (`#ff4000`), `ghost` (`#f4f4ff`), `blood` (`#ff1000`) |

Any other colour as `#rrggbb`. Periods are accepted from 0.2 to 600 seconds. The smoke shader mixes some grey into
every colour, so strongly saturated colours (one channel at full, another near zero) keep their look best; pale or
mixed tones come out greyish.

After a level loads the dll scans the game memory for the smoke colour objects at about 4 s and 20 s; a scan takes
1 to 2 seconds on the dll's own thread, not on a game thread. A new colour becomes visible once the scan has found
the objects (`/smoke status` shows the counts). Smoke colours are your own view only.

**Note on `/smoke per`:** this is the one mode that writes a single call instruction in the game's code in memory
(the hook that gives each grenade effect its own colour). It is installed the first time the mode is used and left
in place until the game exits; `/smoke off` only disables it. All other modes write nothing to code.

## Where the files live

Everything is in `%LOCALAPPDATA%\BC2ChatSmoke\` (paste that into the Explorer address bar), not in the game folder,
because the game folder under Program Files is read-only for the game:

| File | Purpose |
|---|---|
| `muted_players.txt` | Mute list (one entry per line, `#` / `;` comments). Created empty on the first run. |
| `hashed_players.txt` | Hash list, same format. |
| `config.ini` | `[dll] chat=off` disables everything; `[smoke]` keys `color=`, `color2=`, `mode=`, `period=`, `per=` keep the smoke setting. |
| `logs\chat_<pid>.log` | One log per game process; the first line is the dll version. Attach it to bug reports. |

**Migration from bc2_mute:** on the first run, if an old `muted_players.txt` lies next to the dll in the game folder
and the new list does not exist yet, it is copied once; an existing new list is never overwritten. If the game ran
without administrator rights, Windows may have stored the old list under
`%LOCALAPPDATA%\VirtualStore\Program Files...\<game folder>\` instead; copy it to `%LOCALAPPDATA%\BC2ChatSmoke\`
by hand in that case.

## How it works

- `BFBC2Game.exe` imports `msimg32.dll` and loads the copy from its own folder before the system one. This dll
  forwards the five real exports (`AlphaBlend`, `DllInitialize`, `GradientFill`, `TransparentBlt`, `vSetDdrawflag`)
  to `System32\msimg32.dll`, so the game's drawing is unchanged.
- Only inside `BFBC2Game.exe` it starts one monitor thread (250 ms). In any other process it does nothing but forward.
- **Chat hooks** are instance-vtable clone hooks on two game objects: the UI event dispatcher (incoming chat) and the
  chat input submit handler (outgoing chat). The object's vtable pointer is swapped for a dll-owned copy in which one
  slot points into the dll. No bytes of the exe on disk or of its code in memory are changed (the only exception is
  the opt-in `/smoke per` hook described above). The monitor thread re-attaches whenever the game recreates the UI.
- **Smoke colours** are a memory scan for the game's `PolynomialColorInterpData` colour objects of the three smoke
  emitters, matched by their vtable and their exact original grey values, so nothing else can match. The dll then
  writes small float colour values into those objects, re-validating each object before every write; `/smoke off`
  writes the originals back.
- Every read of game memory is guarded with structured exception handling (`__try`), so a vanished object cannot
  crash the game.

## Compatibility

- Retail / Project Rome `BFBC2Game.exe`, SHA-256
  `3911fcc8914b0158c434cee36f19295316e715d4c9a0e70c857e8c39e3d54258`.
- Steam `BFBC2Game.exe` (under `steamapps\common`), SHA-256
  `22e96001382eb847d3ab0a02b59ec7bb79ba47e04146e74c6c49e38465f38c96`, supported since 0.1.2. It is a separate
  compilation of the same game version (SteamStub-wrapped), so its addresses differ; the dll carries both address sets
  and picks the matching one at start-up (the log says `exe profile: retail` or `exe profile: steam`). The Steam
  addresses were derived offline from that exe and are **not yet confirmed in a live game**: reports welcome.
  Check your exe with `certutil -hashfile BFBC2Game.exe SHA256`.
- The dll checks the expected vtable bytes first and stays inactive on any other exe (the log then says
  `unknown exe (PE stamp ..., neither retail nor Steam); chat hooks inactive`). Nothing is hooked in that case.
- 32-bit (x86) only, like the game. Windows 10/11.

## Build from source

Requirements: Visual Studio 2022 (or 2019) with the C++ x86 build tools.

```powershell
powershell -ExecutionPolicy Bypass -File build.ps1
```

The script locates `vcvars32.bat` through `vswhere`, then:

1. builds `build\msimg32.dll` with `/O2 /W4 /GS- /LD`, static CRT, exports from `src\msimg32.def`;
2. builds and **runs** `client_chat_selftest.exe`, the offline command-layer tests (about 80 checks: parser, list
   round trips, hash transform, colour parsing and mapping, object matcher, animation, per-grenade clone logic). A
   failing check fails the build; no game is needed;
3. writes `dist\bc2-chat-smoke-<version>.zip` (`msimg32.dll`, `README.md`, `LICENSE`), `dist\msimg32.dll` and
   `dist\SHA256.txt`.

Source layout:

| File | Role |
|---|---|
| `src\msimg32.c` | Loader: system `msimg32` forwarders, exe-name gate, mute-list migration, `DllMain`. |
| `src\msimg32.def` | The five exports. |
| `src\version.h` | `BC2_VERSION_STRING`. |
| `src\standalone.h` | Force-included build defines: `BC2_CHAT_STANDALONE`, `BC2_CHAT_APPDIR "BC2ChatSmoke"`, `BC2_CHAT_TAG "[chat]"`, `BC2_CHAT_LOG_PREFIX`. |
| `src\client_chat.c/.h` | Chat hooks, lists, command parser. |
| `src\client_smoke.c/.h`, `src\client_smoke_hook.c/.h` | Smoke scan, colour writes, animation, per-grenade hook. |
| `src\client_chat_selftest.c` | The offline tests run by `build.ps1`. |

The chat and smoke modules are byte-identical copies of the same files in the BC2 Local Replay project's
`rome_proxy` folder; the standalone behaviour comes only from the defines in `src\standalone.h`. To re-sync, copy
the files over.

GitHub Actions (`.github/workflows/build.yml`) runs the same `build.ps1` on every push and publishes a Release with
the zip and `SHA256.txt` on every tag `v*`.

## Release verification

`SHA256.txt` next to each release lists one line per file, `<sha256>  <file>`, for the zip and for `msimg32.dll`.
Compare it with the hash of what you downloaded:

```powershell
Get-FileHash .\bc2-chat-smoke-0.1.2.zip -Algorithm SHA256
Get-FileHash .\msimg32.dll -Algorithm SHA256
```

or

```bat
certutil -hashfile bc2-chat-smoke-0.1.2.zip SHA256
certutil -hashfile msimg32.dll SHA256
```

The hashes must match the lines in `SHA256.txt`. The same hashes are printed at the end of the GitHub Actions build
log of the release, so you can check that the release files are the ones the public CI produced.

## Troubleshooting

- `/help` shows nothing: check `%LOCALAPPDATA%\BC2ChatSmoke\logs\chat_<pid>.log`. "vtable bytes differ" means a
  different exe; "off (ini chat=off)" means `config.ini` disabled the dll.
- Smoke stays grey: wait for the scan after the level load, then `/smoke status`; `0 obj` means nothing was found
  yet, the dll keeps rescanning slowly.
- Game does not start or behaves oddly: make sure only one of this `msimg32.dll` and the BC2 Local Replay
  `dinput8.dll` is installed.
- Bug reports: include the log file and the output of `/smoke status`.
