# Build bc2-chat-smoke: msimg32.dll (x86) + the offline selftest, then package dist\.
# Needs Visual Studio 2019/2022 with the C++ x86 build tools (vcvars32.bat, located via vswhere).
# The selftest (client_chat_selftest.exe) is built and RUN here; a failing check fails the build (this is the CI test).
param(
    [string]$BuildDir = "build",
    [string]$OutDir = "dist"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path

# ---- toolchain -------------------------------------------------------------------------------------------------
$vcvars = $null
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($installPath) {
        $candidate = Join-Path $installPath "VC\Auxiliary\Build\vcvars32.bat"
        if (Test-Path $candidate) { $vcvars = $candidate }
    }
}
if (-not $vcvars) {
    $globs = @("${env:ProgramFiles}\Microsoft Visual Studio\*\*\VC\Auxiliary\Build\vcvars32.bat",
               "${env:ProgramFiles(x86)}\Microsoft Visual Studio\*\*\VC\Auxiliary\Build\vcvars32.bat")
    $found = Get-ChildItem -Path $globs -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) { $vcvars = $found.FullName }
}
if (-not $vcvars) { throw "vcvars32.bat not found: install the Visual Studio C++ x86 build tools." }

# ---- version ---------------------------------------------------------------------------------------------------
$versionLine = Select-String -Path (Join-Path $Root "src\version.h") -Pattern 'BC2_VERSION_STRING\s+"([^"]+)"'
if (-not $versionLine) { throw "BC2_VERSION_STRING not found in src\version.h" }
$version = $versionLine.Matches[0].Groups[1].Value
if ($env:GITHUB_REF_TYPE -eq "tag" -and $env:GITHUB_REF_NAME -ne "v$version") {
    throw "tag $env:GITHUB_REF_NAME does not match src\version.h ($version)"
}

# ---- folders ---------------------------------------------------------------------------------------------------
$build = Join-Path $Root $BuildDir
$dist  = Join-Path $Root $OutDir
$stage = Join-Path $build "stage"
foreach ($d in @($build, $dist)) { if (Test-Path $d) { Remove-Item -Recurse -Force $d } }
foreach ($d in @($build, $dist, $stage, (Join-Path $build "obj_dll"), (Join-Path $build "obj_test"))) {
    New-Item -ItemType Directory -Force -Path $d | Out-Null
}

# ---- compile (one cmd script: vcvars32 + two cl calls; no quoting through three shells) ---------------------------
$modules = "src\client_chat.c src\client_smoke.c src\client_smoke_hook.c src\client_fx.c"
$flags   = "/nologo /Brepro /O2 /W4 /GS- /Isrc /FIstandalone.h"
$cmd = @(
    "@echo off",
    "call `"$vcvars`" >nul 2>&1 || exit /b 1",
    "cd /d `"$Root`" || exit /b 1",
    "cl $flags /LD src\msimg32.c $modules /Fo:`"$build\obj_dll\\`" /Fe:`"$build\msimg32.dll`" /link /Brepro /DEF:src\msimg32.def kernel32.lib user32.lib || exit /b 1",
    "cl $flags /DBC2_CHAT_SELFTEST src\client_chat_selftest.c $modules /Fo:`"$build\obj_test\\`" /Fe:`"$build\client_chat_selftest.exe`" /link /Brepro kernel32.lib user32.lib || exit /b 1"
)
$cmdFile = Join-Path $build "compile.cmd"
[System.IO.File]::WriteAllLines($cmdFile, $cmd)
& cmd.exe /c $cmdFile
if ($LASTEXITCODE -ne 0) { throw "compilation failed (exit $LASTEXITCODE)" }
$dll = Join-Path $build "msimg32.dll"
if (-not (Test-Path $dll)) { throw "msimg32.dll was not produced" }

# ---- selftest (no game needed) ---------------------------------------------------------------------------------
Write-Host "--- client_chat_selftest"
& (Join-Path $build "client_chat_selftest.exe") $build
if ($LASTEXITCODE -ne 0) { throw "client_chat_selftest failed (exit $LASTEXITCODE)" }

# ---- package ---------------------------------------------------------------------------------------------------
Copy-Item $dll (Join-Path $stage "msimg32.dll")
Copy-Item (Join-Path $Root "README.md") (Join-Path $stage "README.md")
Copy-Item (Join-Path $Root "LICENSE") (Join-Path $stage "LICENSE")
$zipName = "bc2-chat-smoke-$version.zip"
$zip = Join-Path $dist $zipName
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip
Copy-Item $dll (Join-Path $dist "msimg32.dll")

$zipHash = (Get-FileHash -Algorithm SHA256 $zip).Hash.ToLowerInvariant()
$dllHash = (Get-FileHash -Algorithm SHA256 $dll).Hash.ToLowerInvariant()
[System.IO.File]::WriteAllLines((Join-Path $dist "SHA256.txt"), @("$zipHash  $zipName", "$dllHash  msimg32.dll"))

Write-Host "Build succeeded (version $version):"
Write-Host "  $dist\msimg32.dll  $dllHash"
Write-Host "  $zip  $zipHash"
Write-Host "  $dist\SHA256.txt"
