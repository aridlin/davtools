param(
    [string]$BuildDir = "build-windows",
    [string]$OpenSslRoot = "C:/msys64/ucrt64",
    [string]$Compiler = "C:/msys64/ucrt64/bin/g++.exe"
)

$ErrorActionPreference = "Stop"

$sourceDir = Split-Path -Parent $PSScriptRoot
$buildPath = Join-Path $sourceDir $BuildDir
$distPath = Join-Path $sourceDir "dist-windows"

function Invoke-Checked {
    param([scriptblock]$Command)
    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE"
    }
}

Invoke-Checked {
cmake -G Ninja `
    -S $sourceDir `
    -B $buildPath `
    -DCMAKE_BUILD_TYPE=Release `
    "-DCMAKE_CXX_COMPILER=$Compiler" `
    "-DOPENSSL_ROOT_DIR=$OpenSslRoot" `
    -DDAVTOOLS_FETCH_BOOST=ON `
    -DDAVTOOLS_BUILD_PANEL=ON `
    -DDAVTOOLS_BUILD_FTHT_PANEL=ON
}

Invoke-Checked { cmake --build $buildPath -j }

$runtimeDlls = @(
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll",
    "libcrypto-3-x64.dll"
)

foreach ($dll in $runtimeDlls) {
    $source = Join-Path $OpenSslRoot "bin/$dll"
    if (Test-Path $source) {
        Copy-Item -LiteralPath $source -Destination $buildPath -Force
    }
}

New-Item -ItemType Directory -Force -Path $distPath | Out-Null
Copy-Item -LiteralPath (Join-Path $buildPath "convertdav.exe") -Destination $distPath -Force
Copy-Item -LiteralPath (Join-Path $buildPath "convertdav-panel.exe") -Destination $distPath -Force
Copy-Item -LiteralPath (Join-Path $buildPath "convertdav-ftht-panel.exe") -Destination $distPath -Force

foreach ($dll in $runtimeDlls) {
    $source = Join-Path $buildPath $dll
    if (Test-Path $source) {
        Copy-Item -LiteralPath $source -Destination $distPath -Force
    }
}

Write-Host "Built:"
Write-Host "  $buildPath/convertdav.exe"
Write-Host "  $buildPath/convertdav-panel.exe"
Write-Host "  $buildPath/convertdav-ftht-panel.exe"
Write-Host "Packaged:"
Write-Host "  $distPath"
