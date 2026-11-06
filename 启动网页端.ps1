param([string]$DataDir=(Join-Path $PSScriptRoot 'zuoyezuihou'),[int]$Port=3000,[switch]$Lan,[switch]$Demo,[switch]$BuildOnly)
$ErrorActionPreference='Stop'
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$setup='call "{0}\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64 >nul && set' -f $vs
& cmd /d /c $setup | ForEach-Object { $i=$_.IndexOf('='); if($i -gt 0){[Environment]::SetEnvironmentVariable($_.Substring(0,$i),$_.Substring($i+1),'Process')} }
$dest=Join-Path $PSScriptRoot ('build/web-'+[DateTime]::Now.ToString('yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Force $dest | Out-Null
$sources=@(Get-ChildItem (Join-Path $PSScriptRoot 'zuoyezuihou') -Filter *.c | Where-Object {$_.Name -notin @('main.c','console_password_test.c')} | ForEach-Object FullName)
Push-Location $dest
try { & cl /nologo /utf-8 /D_CRT_SECURE_NO_WARNINGS /I"$PSScriptRoot/zuoyezuihou" @sources "$PSScriptRoot/native-web/worker.c" "$PSScriptRoot/native-web/cJSON.c" /Fe:sgms-worker.exe /link bcrypt.lib; if($LASTEXITCODE){throw 'C网页进程编译失败'} } finally {Pop-Location}
Push-Location (Join-Path $PSScriptRoot 'web')
try {if(!(Test-Path node_modules)){ & npm install --cache "$PSScriptRoot/build/npm-cache" --no-audit --no-fund; if($LASTEXITCODE){throw '依赖安装失败'}}; & npm run build; if($LASTEXITCODE){throw '网页构建失败'}; if($BuildOnly){return}; if($Demo){$DataDir=Join-Path $dest 'demo';New-Item -ItemType Directory -Force $DataDir | Out-Null}; $env:SGMS_DEMO=if($Demo){"1"}else{"0"}; $env:SGMS_WORKER=Join-Path $dest 'sgms-worker.exe';$env:SGMS_DATA=(Resolve-Path -LiteralPath $DataDir).Path;$env:PORT="$Port";$env:HOST=if($Lan){'0.0.0.0'}else{'127.0.0.1'}; & node server.mjs } finally {Pop-Location}
