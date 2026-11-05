param(
    [Parameter(Mandatory)][string]$RuntimeDirectory,
    [Parameter(Mandatory)][string]$OutputRoot,
    [ValidateRange(1, 20)][int]$Repetitions = 3,
    [int[]]$Workers = @(0, 1, 4),
    [int[]]$BodyCounts = @(128, 512, 2048)
)
$ErrorActionPreference = 'Stop'
if (Test-Path -LiteralPath $OutputRoot) { throw 'Use a fresh output directory.' }
$RuntimeDirectory = (Resolve-Path -LiteralPath $RuntimeDirectory).Path
New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null
$configurations = @(@{ name = 'ODE'; exe = 'xrODEPhysicsBenchmark.exe'; workers = 0 })
foreach ($workerCount in $Workers) {
    if ($workerCount -lt 0 -or $workerCount -gt 32) { throw 'Workers must be 0..32.' }
    $configurations += @{ name = "JoltNative-$workerCount"; exe = 'xrNativePhysicsBenchmark.exe'; workers = $workerCount }
}
$sourceRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$metadata = [ordered]@{
    source_commit = (& git -C $sourceRoot rev-parse HEAD)
    source_changes = @(& git -C $sourceRoot status --porcelain)
    cpu = @(Get-CimInstance Win32_Processor | Select-Object Name, NumberOfCores, NumberOfLogicalProcessors)
    time_utc = [DateTime]::UtcNow.ToString('o')
    hashes = @(Get-ChildItem -LiteralPath $RuntimeDirectory -File | Where-Object { $_.Name -in @(
        'xrNativePhysicsBenchmark.exe', 'xrODEPhysicsBenchmark.exe', 'xrPhysicsCore.dll', 'ODE.dll', 'xrCore.dll') } |
        ForEach-Object { @{ file = $_.Name; sha256 = (Get-FileHash -LiteralPath $_.FullName).Hash } })
    body_counts = $BodyCounts; workers = $Workers; repetitions = $Repetitions
    warmup_steps = 100; timed_steps = 500; timestep_ms = 10; iterations = 18
}
$metadata | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $OutputRoot 'metadata.json')
$previousWorkers = $env:XRAY_JOLT_WORKERS
try {
    for ($repetition = 1; $repetition -le $Repetitions; ++$repetition) {
        $order = @($configurations)
        $offset = ($repetition - 1) % $order.Count
        $order = @($order[$offset..($order.Count-1)]) + @($order | Select-Object -First $offset)
        foreach ($scenario in 'stacks','chains') {
            foreach ($count in $BodyCounts) {
                foreach ($configuration in $order) {
                    $env:XRAY_JOLT_WORKERS = [string]$configuration.workers
                    $stem = "$($configuration.name)-$scenario-$count-run-$repetition"
                    $output = Join-Path $OutputRoot "$stem.csv"
                    Write-Host "Running $stem"
                    & (Join-Path $RuntimeDirectory $configuration.exe) $scenario $count $output > (Join-Path $OutputRoot "$stem.log") 2>&1
                    if ($LASTEXITCODE -ne 0) { throw "Benchmark failed: $stem (exit $LASTEXITCODE)" }
                    $rows = @(Import-Csv -LiteralPath $output)
                    if ($rows.Count -ne 500) { throw "Incomplete capture: $stem" }
                }
            }
        }
    }
} finally { $env:XRAY_JOLT_WORKERS = $previousWorkers }
