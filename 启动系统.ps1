$ErrorActionPreference = 'Stop'
$project = Join-Path $PSScriptRoot 'zuoyezuihou'
$candidates = @(Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'build') -Directory -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -like '*Release-x64' } | Sort-Object Name -Descending |
    ForEach-Object { Join-Path $_.FullName 'zuoyezuihou.exe' } | Where-Object { Test-Path -LiteralPath $_ })
if ($candidates.Count -eq 0) { throw '请先在项目根目录运行 ./build-test.ps1 -Configuration Release -BuildOnly' }
Start-Process -FilePath $candidates[0] -WorkingDirectory $project -WindowStyle Normal
