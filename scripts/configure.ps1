param(
  [string]$Workspace
)
$ErrorActionPreference = 'Stop'
if (-not $Workspace) { $Workspace = Split-Path $PSScriptRoot -Parent }
$src = Join-Path $Workspace 'benchmark'
$bld = Join-Path $src 'build-vs'
Import-Module "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
# Auto-detect VS DevShell Instance ID from _Instances folder
$instancesRoot = "C:\ProgramData\Microsoft\VisualStudio\Packages\_Instances"
$vsId = $null
if (Test-Path $instancesRoot) {
  try {
    $vsId = Get-ChildItem -Path $instancesRoot -Directory -ErrorAction Stop |
      Select-Object -First 1 -ExpandProperty Name
  } catch { }
}
if ($vsId) {
  Enter-VsDevShell $vsId -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64"
} else {
  Write-Host "Warning: No VS instance ID found under $instancesRoot. Falling back to automatic location." -ForegroundColor Yellow
  Enter-VsDevShell -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64"
}
Set-Location $src
if (!(Test-Path $bld)) { New-Item -ItemType Directory -Force -Path $bld | Out-Null }
$args = @('-S', $src, '-B', $bld, '-G', 'Visual Studio 17 2022')
if ($env:VCPKG_ROOT) {
  $tool = Join-Path $env:VCPKG_ROOT 'scripts/buildsystems/vcpkg.cmake'
  if (Test-Path $tool) { $args += "-DCMAKE_TOOLCHAIN_FILE=$tool" }
}
cmake @args
