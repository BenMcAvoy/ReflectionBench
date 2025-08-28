param(
  [string]$Workspace
)
$ErrorActionPreference = 'Stop'
if (-not $Workspace) { $Workspace = Split-Path $PSScriptRoot -Parent }
$root = Join-Path $Workspace 'benchmark'
$exeVS = Join-Path $root 'build-vs/Release/meta_bench.exe'
$exeNinja = Join-Path $root 'build/meta_bench.exe'
$exe = if (Test-Path $exeVS) { $exeVS } elseif (Test-Path $exeNinja) { $exeNinja } else { '' }
if (-not $exe) { throw 'Executable not found. Build the project first.' }
$py = 'py'
$req = Join-Path $root 'tools/requirements.txt'
& $py -m pip install --user -r $req | Out-Null
& $exe | & $py (Join-Path $root 'tools/render_table.py')
