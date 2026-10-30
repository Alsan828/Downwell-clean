param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [ValidateSet('x64', 'Win32')][string]$Platform = 'x64'
)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'setup-dxlib.ps1')
$vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswherePath)) { throw 'Visual Studio Installer not found.' }
$msbuildPath = & $vswherePath -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuildPath) { throw 'Visual Studio C++ build tools not found.' }
$solutionPlatform = if ($Platform -eq 'Win32') { 'x86' } else { 'x64' }
& $msbuildPath (Join-Path $PSScriptRoot 'Project1.sln') /m /nologo /verbosity:minimal "/p:Configuration=$Configuration" "/p:Platform=$solutionPlatform"
if ($LASTEXITCODE -ne 0) { throw "Build failed: $LASTEXITCODE" }
