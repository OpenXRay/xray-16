param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [ValidateSet('Both', 'OPCODE', 'Jolt')]
    [string]$Backend = 'Both',
    [string]$BuildRoot,
    [string]$CMake = 'cmake',
    [string]$NuGet = 'nuget',
    [switch]$Benchmark,
    [ValidateRange(1, 64)]
    [int]$Jobs = 4
)

$ErrorActionPreference = 'Stop'
if ($Benchmark -and $Configuration -ne 'Release') {
    throw 'Use -Configuration Release for benchmark measurements.'
}
$sourceRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if (!$BuildRoot) { $BuildRoot = Join-Path $sourceRoot 'build/windows-collision' }
$BuildRoot = [IO.Path]::GetFullPath($BuildRoot)
$logRoot = Join-Path $BuildRoot "logs/$Configuration"
New-Item -ItemType Directory -Force -Path $logRoot | Out-Null

$cmakeExe = (Get-Command $CMake -ErrorAction Stop).Source
$ctestExe = Join-Path (Split-Path $cmakeExe) 'ctest.exe'
$nugetExe = (Get-Command $NuGet -ErrorAction Stop).Source
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vsPath = & $vswhere -latest -products '*' -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vsPath) { throw 'Visual Studio 2022 C++ tools and a Windows SDK are required.' }
$msbuild = Join-Path $vsPath 'MSBuild/Current/Bin/MSBuild.exe'

function Invoke-Logged([string]$Executable, [string[]]$Arguments, [string]$Name) {
    $log = Join-Path $logRoot "$Name.log"
    Write-Host "$Name (log: $log)"
    # Windows PowerShell represents redirected native stderr as error records,
    # including harmless compiler warnings. Use the process exit code instead.
    $ErrorActionPreference = 'Continue'
    & $Executable @Arguments *> $log
    $code = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    if ($code -ne 0) {
        Get-Content -LiteralPath $log -Tail 60 | Write-Host
        throw "$Name failed with exit code $code. See $log"
    }
}

Push-Location $sourceRoot
try {
    Invoke-Logged $nugetExe @('restore', 'src/engine.sln', '-NonInteractive') 'nuget'
    Invoke-Logged $msbuild @('src/engine.sln', "/m:$Jobs", "/p:CL_MPCount=$Jobs",
        "/p:Configuration=$Configuration", '/p:Platform=x64', '/v:minimal') 'native-build'

    $backends = if ($Backend -eq 'Both') { @('OPCODE', 'Jolt') } else { @($Backend) }
    foreach ($selectedBackend in $backends) {
        $buildDir = Join-Path $BuildRoot "$selectedBackend/$Configuration"
        $runtimeDir = Join-Path $buildDir "bin/$Configuration"
        New-Item -ItemType Directory -Force -Path $runtimeDir | Out-Null

        # Each backend gets its own runtime, including the native engine and
        # dependencies. Never overwrite the collision/physics DLLs built below,
        # even when rerunning incrementally after another native build.
        Get-ChildItem -LiteralPath (Join-Path $sourceRoot "bin/x64/$Configuration") -File |
            Where-Object { $_.Name -notmatch '^xr(CDB|Physics)\.' } |
            Copy-Item -Destination $runtimeDir -Force

        $jolt = if ($selectedBackend -eq 'Jolt') { 'ON' } else { 'OFF' }
        Invoke-Logged $cmakeExe @('-S', 'misc/windows/collision', '-B', $buildDir,
            '-G', 'Visual Studio 17 2022', '-A', 'x64', "-DCMAKE_GENERATOR_INSTANCE=$vsPath",
            "-DXRAY_NATIVE_CONFIGURATION=$Configuration", "-DXRAY_USE_JOLT_CDB=$jolt") "$selectedBackend-configure"
        Invoke-Logged $cmakeExe @('--build', $buildDir, '--config', $Configuration,
            '--parallel', "$Jobs") "$selectedBackend-build"
        Invoke-Logged $ctestExe @('--test-dir', $buildDir, '-C', $Configuration,
            '--output-on-failure', '--no-tests=error', '-R', '^xrCDB\.') "$selectedBackend-tests"
        Get-Content -LiteralPath (Join-Path $logRoot "$selectedBackend-tests.log") | Write-Host
        $revision = & git rev-parse HEAD
        if ($LASTEXITCODE -ne 0) { throw 'Could not identify the source revision.' }
        [ordered]@{
            backend = $selectedBackend
            configuration = $Configuration
            source_commit = $revision
            # Local edits can affect this build even when HEAD is unchanged.
            source_changes = @(& git status --porcelain --untracked-files=normal)
            visual_studio = $vsPath
            built_at_utc = [DateTime]::UtcNow.ToString('o')
        } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $runtimeDir 'collision-build.json')
        if ($Benchmark) {
            Push-Location $runtimeDir
            try {
                Invoke-Logged (Join-Path $runtimeDir 'xrCDBBenchmark.exe') @(
                    (Join-Path $logRoot "$selectedBackend-benchmark.json")) "$selectedBackend-benchmark"
                Get-Content -LiteralPath (Join-Path $logRoot "$selectedBackend-benchmark.json") | Write-Host
            }
            finally { Pop-Location }
        }
        Write-Host "$selectedBackend runtime: $runtimeDir"
    }
}
finally {
    Pop-Location
}
