[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [ValidateRange(1, 64)]
    [int]$Jobs = [Math]::Min(8, [Environment]::ProcessorCount),

    [string]$Target,

    [switch]$Clean,
    [switch]$ConfigureOnly,
    [switch]$WarningsAsErrors,
    [switch]$Sanitizers,
    [switch]$VerboseBuild
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$preset = $Configuration.ToLowerInvariant()
$buildDirectory = Join-Path (Join-Path $repoRoot "build") $preset

$cmakeCandidates = @(
    (Join-Path $env:ProgramFiles "CMake\bin\cmake.exe"),
    (Join-Path $env:ProgramFiles "Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe")
)
$cmake = $cmakeCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $cmake) {
    $cmake = (Get-Command cmake -ErrorAction Stop).Source
}

if ($Clean -and (Test-Path -LiteralPath $buildDirectory)) {
    & $cmake -E remove_directory $buildDirectory
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to clean '$buildDirectory' (exit code $LASTEXITCODE)"
    }
}

$configureArguments = @("--preset", $preset)

$cCompiler = "C:\msys64\ucrt64\bin\gcc.exe"
$cxxCompiler = "C:\msys64\ucrt64\bin\g++.exe"
if ((Test-Path -LiteralPath $cCompiler) -and (Test-Path -LiteralPath $cxxCompiler)) {
    $configureArguments += "-DCMAKE_C_COMPILER=$cCompiler"
    $configureArguments += "-DCMAKE_CXX_COMPILER=$cxxCompiler"
}

if ($WarningsAsErrors) {
    $configureArguments += "-DSAILING_WARNINGS_AS_ERRORS=ON"
}
if ($Sanitizers) {
    $configureArguments += "-DSAILING_ENABLE_SANITIZERS=ON"
}

Push-Location $repoRoot
try {
    & $cmake @configureArguments
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed with exit code $LASTEXITCODE"
    }

    if ($ConfigureOnly) {
        return
    }

    $buildArguments = @("--build", "--preset", $preset, "--parallel", $Jobs)
    if ($Target) {
        $buildArguments += @("--target", $Target)
    }
    if ($VerboseBuild) {
        $buildArguments += "--verbose"
    }

    & $cmake @buildArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE"
    }
} finally {
    Pop-Location
}
