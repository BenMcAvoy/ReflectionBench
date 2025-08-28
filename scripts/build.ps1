param(
  [string]$Workspace
)
$ErrorActionPreference = 'Stop'
if (-not $Workspace) { $Workspace = Split-Path $PSScriptRoot -Parent }
$src = Join-Path $Workspace 'benchmark'
$bld = Join-Path $src 'build-vs'
Import-Module "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell 63a93980 -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64"
Set-Location $src
if (!(Test-Path (Join-Path $bld 'meta_bench.sln'))) {
  cmake -S $src -B $bld -G "Visual Studio 17 2022"
}
msbuild (Join-Path $bld 'meta_bench.sln') /p:Configuration=Release /m
