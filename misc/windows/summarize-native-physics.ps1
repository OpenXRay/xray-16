param([Parameter(Mandatory)][string]$ResultsRoot)
$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
function Number($value) { [double]::Parse($value, $culture) }
function Median($values) {
    $sorted = @($values | Sort-Object)
    if (!$sorted.Count) { throw 'Empty sample.' }
    $middle = [int][Math]::Floor($sorted.Count / 2)
    if ($sorted.Count % 2) { return $sorted[$middle] }
    return ($sorted[$middle - 1] + $sorted[$middle]) / 2
}
$runs = @(Get-ChildItem -LiteralPath $ResultsRoot -Filter '*-run-*.csv' | ForEach-Object {
    $rows = @(Import-Csv -LiteralPath $_.FullName)
    if ($rows.Count -ne 500) { throw "Incomplete capture: $($_.Name)" }
    $first = $rows[0]
    $times = @($rows | ForEach-Object { Number $_.total_ms } | Sort-Object)
    if (@($rows | Where-Object { $_.backend -ne $first.backend -or $_.scenario -ne $first.scenario -or
        $_.bodies -ne $first.bodies -or $_.workers -ne $first.workers -or $_.dt_ms -ne '10' }).Count) {
        throw "Mixed capture: $($_.Name)"
    }
    $log = Get-Content -LiteralPath ([IO.Path]::ChangeExtension($_.FullName, '.log')) -Raw
    if ($log -notmatch 'PASS' -or $log -match 'FAIL') { throw "Failed run: $($_.Name)" }
    $cpu = $null
    if ($log -match 'TIMED_PROCESS_CPU_MS\s+([0-9.]+)') { $cpu = (Number $Matches[1]) / 500 }
    [pscustomobject]@{
        file = $_.Name; backend = $first.backend; workers = [int]$first.workers
        scenario = $first.scenario; bodies = [int]$first.bodies
        mean_ms = ($times | Measure-Object -Average).Average
        median_ms = Median $times; p95_ms = $times[[int][Math]::Ceiling(.95 * $times.Count) - 1]
        process_cpu_ms_per_step = $cpu
        mean_contacts = ($rows | ForEach-Object { Number $_.contacts } | Measure-Object -Average).Average
        minimum_y = ($rows | ForEach-Object { Number $_.min_y } | Measure-Object -Minimum).Minimum
        maximum_y = ($rows | ForEach-Object { Number $_.max_y } | Measure-Object -Maximum).Maximum
        peak_working_set_mib = ($rows | ForEach-Object { Number $_.working_set_bytes } | Measure-Object -Maximum).Maximum / 1MB
        peak_private_mib = ($rows | ForEach-Object { Number $_.private_bytes } | Measure-Object -Maximum).Maximum / 1MB
    }
})
if (!$runs.Count) { throw 'No captures found.' }
$summary = @($runs | Group-Object backend,workers,scenario,bodies | ForEach-Object {
    $first = $_.Group[0]
    [pscustomobject]@{
        backend = $first.backend; workers = $first.workers; scenario = $first.scenario; bodies = $first.bodies
        runs = $_.Count; median_run_mean_ms = Median $_.Group.mean_ms
        median_run_p95_ms = Median $_.Group.p95_ms
        median_run_process_cpu_ms_per_step = $(if (@($_.Group | Where-Object { $null -eq $_.process_cpu_ms_per_step }).Count) {
            $null
        } else { Median $_.Group.process_cpu_ms_per_step })
        median_run_contacts = Median $_.Group.mean_contacts
        median_peak_working_set_mib = Median $_.Group.peak_working_set_mib
        median_peak_private_mib = Median $_.Group.peak_private_mib
        ode_wall_time_ratio = $null
    }
})
foreach ($entry in $summary) {
    $baseline = @($summary | Where-Object { $_.backend -eq 'ODE' -and $_.scenario -eq $entry.scenario -and $_.bodies -eq $entry.bodies })
    if ($baseline.Count -ne 1) { throw 'Missing or duplicate ODE baseline.' }
    $entry.ode_wall_time_ratio = $entry.median_run_mean_ms / $baseline[0].median_run_mean_ms
}
$runs | Export-Csv -LiteralPath (Join-Path $ResultsRoot 'run-summary.csv') -NoTypeInformation
$summary | Sort-Object scenario,bodies,backend,workers | Export-Csv -LiteralPath (Join-Path $ResultsRoot 'summary.csv') -NoTypeInformation
$summary | Sort-Object scenario,bodies,backend,workers | Format-Table backend,workers,scenario,bodies,runs,median_run_mean_ms,ode_wall_time_ratio -AutoSize
