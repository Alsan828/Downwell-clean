param([string]$ArchivePath)
$ErrorActionPreference = 'Stop'
$taskVersion = '3.25a'
$taskExpectedHash = 'DB9E6138C1665C9D9081DD04019219F332A1185448162FDBBC6F8182B9E4A847'
$taskLibrary = Join-Path $PSScriptRoot 'third_party\DxLib'
$taskNames = @('DxLib.h','DxCompileConfig.h','DxDataType.h','DxDataTypeWin.h','DxFunctionWin.h','DxLib.txt')
foreach ($taskPrefix in @('DxLib','DxLibW','DxUseCLib','DxUseCLibW','DxDrawFunc','libjpeg','libpng','zlib','libtiff','ogg_static','vorbis_static','vorbisfile_static','libtheora_static','celt','opus','opusfile','silk_common','libbulletmath','libbulletdynamics','libbulletcollision')) {
    foreach ($taskArchitecture in @('x86','x64')) {
        foreach ($taskRuntime in @('MT','MTd')) {
            $taskNames += "${taskPrefix}_vs2015_${taskArchitecture}_${taskRuntime}.lib"
        }
    }
}
$taskMissing = @($taskNames | Where-Object { -not (Test-Path -LiteralPath (Join-Path $taskLibrary $_) -PathType Leaf) })
$taskVersionFile = Join-Path $taskLibrary 'version.txt'
if (-not $ArchivePath -and $taskMissing.Count -eq 0 -and
    (Test-Path -LiteralPath $taskVersionFile) -and
    (Get-Content -LiteralPath $taskVersionFile -Raw).Trim() -eq "$taskVersion $taskExpectedHash") {
    Write-Host "DxLib $taskVersion is ready."
    return
}
if (-not $ArchivePath) {
    $taskCache = Join-Path $PSScriptRoot '.deps'
    New-Item -ItemType Directory -Path $taskCache -Force | Out-Null
    $ArchivePath = Join-Path $taskCache 'DxLib_VC3_25a.zip'
    if (-not (Test-Path -LiteralPath $ArchivePath)) {
        Write-Host 'Downloading the official DxLib Visual Studio package...'
        Invoke-WebRequest -Uri 'https://dxlib.xsrv.jp/DxLib/DxLib_VC3_25a.zip' -OutFile $ArchivePath
    }
}
if ((Get-FileHash -LiteralPath $ArchivePath -Algorithm SHA256).Hash -ne $taskExpectedHash) {
    throw 'DxLib package checksum differs from the pinned 3.25a package.'
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
$taskZip = [System.IO.Compression.ZipFile]::OpenRead([System.IO.Path]::GetFullPath($ArchivePath))
try {
    $taskHeader = $taskZip.Entries | Where-Object { $_.FullName.EndsWith('/DxLib.h') } | Select-Object -First 1
    if (-not $taskHeader) { throw 'DxLib.h is missing from the package.' }
    $taskRoot = $taskHeader.FullName.Substring(0, $taskHeader.FullName.Length - 'DxLib.h'.Length)
    New-Item -ItemType Directory -Path $taskLibrary -Force | Out-Null
    foreach ($taskName in $taskNames) {
        $taskEntry = $taskZip.GetEntry($taskRoot + $taskName)
        if (-not $taskEntry) { throw "Package file is missing: $taskName" }
        [System.IO.Compression.ZipFileExtensions]::ExtractToFile($taskEntry, (Join-Path $taskLibrary $taskName), $true)
    }
    Set-Content -LiteralPath $taskVersionFile -Value "$taskVersion $taskExpectedHash" -Encoding ascii
} finally { $taskZip.Dispose() }
Write-Host "DxLib $taskVersion installed in $taskLibrary."
