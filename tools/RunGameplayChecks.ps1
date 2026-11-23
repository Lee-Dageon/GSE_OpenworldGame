param([string[]]$Names = @('NavigationCheck','WorldCombatCheck','WeaponMotionCheck','ExpansionCheck','WorldCompletionCheck','EndgameCheck','StoryFlowCheck','GameContentCheck','InventoryCraftingCheck','WorldLayoutCheck','NpcWanderCheck','NpcInteractionCheck','AnimalHuntingCheck','ChatacabraCheck','HudLayoutCheck'))
$ErrorActionPreference='Stop'
$repoRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$outputRoot=Join-Path $repoRoot 'build/gameplay-checks'
New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null
$vswhere='C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe'
$installation=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$installation) { throw 'Visual Studio C++ tools not found.' }
$developer=Join-Path $installation 'Common7/Tools/VsDevCmd.bat'
$objects=@(Get-ChildItem -LiteralPath (Join-Path $repoRoot 'build/msbuild/obj/x64/Debug') -Filter '*.obj' | Where-Object Name -ne 'SimpleGame.obj')
if ($objects.Count -lt 15) { throw 'Build Debug x64 first.' }
$forced=Join-Path $outputRoot 'CheckRuntime.h'
@'
#include <Windows.h>
#include <crtdbg.h>
#include <cstdlib>
namespace { struct CheckReports { CheckReports() {
    _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE); _CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
} } checkReports; }
'@ | Set-Content -LiteralPath $forced -Encoding utf8
$failed=@()
foreach ($name in $Names) {
    if ($name -notmatch '^[A-Za-z0-9]+Check$') { throw 'Invalid check name.' }
    $source=Join-Path $repoRoot "tests/$name.cpp"
    if (!(Test-Path -LiteralPath $source)) { throw "Missing $source" }
    $exe=Join-Path $outputRoot "$name.exe"
    $response=Join-Path $outputRoot "$name.rsp"
    $arguments=@('/nologo','/std:c++17','/EHsc','/utf-8','/MTd','/D_DEBUG','/DNOMINMAX','/DGLEW_STATIC',
        ('/FI"'+$forced+'"'),('/I"'+(Join-Path $repoRoot 'SimpleGame/Dependencies/glew/include')+'"'),
        ('/I"'+(Join-Path $repoRoot 'SimpleGame/Dependencies/glfw/include')+'"'),
        ('/Fo"'+(Join-Path $outputRoot "$name.obj")+'"'),('/Fe"'+$exe+'"'),('"'+$source+'"'))
    $arguments+=@($objects | ForEach-Object { '"'+$_.FullName+'"' })
    $arguments+=@('/link','/INCREMENTAL:NO','kernel32.lib','user32.lib','gdi32.lib','shell32.lib','ole32.lib','oleaut32.lib','uuid.lib','comdlg32.lib','advapi32.lib','opengl32.lib','gdiplus.lib','winmm.lib','dwmapi.lib')
    $arguments | Set-Content -LiteralPath $response -Encoding utf8
    $commandFile=Join-Path $outputRoot 'compile.cmd'
    @('@echo off',('call "'+$developer+'" -arch=x64 -host_arch=x64 >nul'),'if errorlevel 1 exit /b %errorlevel%',('cl.exe @"'+$response+'"')) | Set-Content -LiteralPath $commandFile -Encoding ascii
    $compileLog=Join-Path $outputRoot "$name-build.log"
    & $env:ComSpec /d /c $commandFile *> $compileLog
    if ($LASTEXITCODE -ne 0) { Get-Content -LiteralPath $compileLog; $failed+=$name; continue }
    Push-Location (Join-Path $repoRoot 'SimpleGame')
    try {
        $log=Join-Path $outputRoot "$name.log"
        $previousPreference=$ErrorActionPreference
        try { $ErrorActionPreference='Continue'; & $exe *> $log; $result=$LASTEXITCODE }
        finally { $ErrorActionPreference=$previousPreference }
        Get-Content -LiteralPath $log
        if ($result -ne 0) { $failed+=$name; Write-Output "FAIL: $name ($result)" }
    } finally { Pop-Location }
}
if ($failed.Count) { throw ('Failed checks: '+($failed -join ', ')) }
Write-Output ('PASS: '+$Names.Count+' gameplay checks')
