param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Debug',
    [ValidateSet('x64','Win32')][string]$Platform = 'x64',
    [switch]$BuildOnly
)
$ErrorActionPreference = 'Stop'
$locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $locator)) { throw 'Environment: vswhere.exe not found; install VS2022 C++ desktop tools.' }
$builder = & $locator -latest -version '[17.0,18.0)' -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (!$builder) { throw 'Environment: VS2022 MSBuild / C++ tools not found.' }
$installation = & $locator -latest -version '[17.0,18.0)' -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$developerCommand = Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
$targetArch = if ($Platform -eq 'Win32') { 'x86' } else { 'x64' }
$setupCommand = 'call "{0}" -no_logo -host_arch=x64 -arch={1} >nul && set' -f $developerCommand,$targetArch
$setupLines = & cmd /d /c $setupCommand
if ($LASTEXITCODE -ne 0) { throw 'Environment: failed to initialise VS developer tools.' }
foreach ($line in $setupLines) {
    $separator = $line.IndexOf('=')
    if ($separator -gt 0) { [Environment]::SetEnvironmentVariable($line.Substring(0,$separator),$line.Substring($separator+1),'Process') }
}

$stamp = [DateTime]::Now.ToString('yyyyMMdd-HHmmss-fff') + "-$Configuration-$Platform"
$output = Join-Path $PSScriptRoot "build\$stamp\"
$logs = Join-Path $PSScriptRoot "test-results\$stamp"
New-Item -ItemType Directory -Force -Path $output,$logs | Out-Null
& $builder (Join-Path $PSScriptRoot 'zuoyezuihou\zuoyezuihou.vcxproj') /t:Build "/p:Configuration=$Configuration" "/p:Platform=$Platform" "/p:OutDir=$output" "/p:IntDir=$($output)obj\" /p:SGMSPortableBuild=true /p:PreferredToolArchitecture=x64 /m /nologo 2>&1 | Tee-Object (Join-Path $logs 'build.txt')
if ($LASTEXITCODE -ne 0) { throw "Build failed; logs: $logs" }
$env:SGMS_EXE = Join-Path $output 'zuoyezuihou.exe'
$env:SGMS_RESULTS = $logs
if (!$BuildOnly) {
    $faultOutput = Join-Path $output 'fault-tests\'
    & $builder (Join-Path $PSScriptRoot 'zuoyezuihou\zuoyezuihou.vcxproj') /t:Build "/p:Configuration=$Configuration" "/p:Platform=$Platform" "/p:OutDir=$faultOutput" "/p:IntDir=$($faultOutput)obj\" /p:ExtraDefines=SGMS_TESTING /p:SGMSPortableBuild=true /p:PreferredToolArchitecture=x64 /m /nologo 2>&1 | Tee-Object (Join-Path $logs 'fault-build.txt')
    if ($LASTEXITCODE -ne 0) { throw "Fault test build failed; logs: $logs" }
    $env:SGMS_FAULT_EXE = Join-Path $faultOutput 'zuoyezuihou.exe'

    foreach ($suite in @('verify_fixes.py','verify_classes.py','verify_tables.py','verify_robustness.py','verify_storage.py','verify_menu_data.py','verify_login.py','verify_clients.py','verify_workflows.py')) {
        & python -X utf8 (Join-Path $PSScriptRoot "zuoyezuihou\$suite") 2>&1 | Tee-Object (Join-Path $logs "$suite.txt")
        if ($LASTEXITCODE -ne 0) { throw "Test failed: $suite; data and logs retained: $logs" }
    }
}
Write-Host "Executable: $env:SGMS_EXE"
Write-Host "Logs: $logs"
