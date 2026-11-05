param([Parameter(Mandatory)][string]$OutputRoot)
$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
function Number($value) { [double]::Parse([string]$value, $culture) }
function Mean($values) { ($values | Measure-Object -Average).Average }
function NativeMean($steps, [string]$field) {
    if (!$steps[0].PSObject.Properties[$field]) { return 0 }
    Mean @($steps | ForEach-Object { Number $_.$field })
}
function Quantile($values, [double]$fraction) {
    $sorted = @($values | Sort-Object)
    if (!$sorted.Count) { throw 'Empty benchmark sample.' }
    $index = [Math]::Max(0, [Math]::Ceiling($fraction * $sorted.Count) - 1)
    $sorted[$index]
}
$runs = @()
$referenceQueries = @{}
foreach ($metadata in Get-ChildItem -LiteralPath (Join-Path $OutputRoot 'results') -Recurse -Filter run.json) {
    $run = Get-Content -LiteralPath $metadata.FullName -Raw | ConvertFrom-Json
    if ($run.exit_code -ne 0) { throw "Failed game process: $($metadata.FullName)" }
    $phases = @()
    foreach ($phase in 'idle','ragdolls') {
        $rows = @(Import-Csv -LiteralPath (Join-Path $metadata.DirectoryName "benchmark-$phase.csv"))
        $steps = @($rows | Where-Object kind -eq step)
        $frames = @($rows | Where-Object kind -eq frame)
        if ($steps.Count -lt 500 -or $frames.Count -lt 100) { throw "Insufficient capture: $($metadata.DirectoryName) / $phase" }
        if (@($rows | Where-Object { $_.collision_backend -ne $run.collision -or $_.dynamics_backend -ne $run.dynamics }).Count) {
            throw 'Runtime manifest disagrees with the backend embedded in the capture.'
        }
        $total = @($steps | ForEach-Object { Number $_.total_ms })
        $solver = @($steps | ForEach-Object { Number $_.solver_ms })
        $collision = @($steps | ForEach-Object { Number $_.collision_ms })
        $intervals = @($frames | ForEach-Object { Number $_.total_ms })
        $dt = @($steps | ForEach-Object { Number $_.dt_ms } | Select-Object -Unique)
        if ($dt.Count -ne 1 -or [Math]::Abs($dt[0] - 10) -gt .001) { throw 'Unexpected or variable physics timestep.' }
        $phases += [pscustomobject]@{
            phase = $phase; steps = $steps.Count; frames = $frames.Count
            mean_total_ms = Mean $total; median_total_ms = Quantile $total .5; p95_total_ms = Quantile $total .95
            mean_solver_ms = Mean $solver; p95_solver_ms = Quantile $solver .95
            mean_collision_ms = Mean $collision
            mean_native_preparation_ms = NativeMean $steps 'native_preparation_ms'
            mean_native_integration_ms = NativeMean $steps 'native_integration_ms'
            mean_native_feedback_ms = NativeMean $steps 'native_feedback_ms'
            mean_bodies = Mean @($steps | ForEach-Object { Number $_.bodies })
            mean_contacts = Mean @($steps | ForEach-Object { Number $_.contacts })
            mean_joints = Mean @($steps | ForEach-Object { Number $_.joints })
            mean_frame_ms = Mean $intervals; p95_frame_ms = Quantile $intervals .95; p99_frame_ms = Quantile $intervals .99
            physics_ms_per_simulated_second = (Mean $total) * 1000 / $dt[0]
        }
    }
    $queries = @()
    foreach ($group in (Import-Csv -LiteralPath (Join-Path $metadata.DirectoryName 'benchmark-queries.csv') | Group-Object workload)) {
        $first = $group.Group[0]
        $signature = "$($first.origin_x),$($first.origin_y),$($first.origin_z),$($first.queries),$($first.hits)"
        if ($referenceQueries.ContainsKey($group.Name)) {
            $reference = $referenceQueries[$group.Name]
            if ($signature -ne $reference.signature -or [Math]::Abs((Number $first.range_sum) - $reference.ranges) -gt .01) {
                throw "Query input/results mismatch: $($run.runtime) / $($group.Name)"
            }
        }
        else { $referenceQueries[$group.Name] = @{ signature = $signature; ranges = (Number $first.range_sum) } }
        if (@($group.Group | Where-Object { $_.hits -ne $first.hits -or $_.range_sum -ne $first.range_sum }).Count) {
            throw 'Query results changed between identical batches.'
        }
        $queries += [pscustomobject]@{
            workload = $group.Name
            median_us_per_query = (Quantile @($group.Group | ForEach-Object { Number $_.total_ms }) .5) * 1000 / (Number $first.queries)
            hits = Number $first.hits; range_sum = Number $first.range_sum
            model_reported_bytes = Number $first.model_bytes
        }
    }
    $runs += [pscustomobject]@{
        runtime = $run.runtime; repetition = $run.repetition; collision = $run.collision; dynamics = $run.dynamics
        maximum_working_set_mib = $run.maximum_observed_working_set_bytes / 1MB
        save_reload_validated = [bool]$run.save_reload_validated
        phases = $phases; queries = $queries
    }
}
if (!$runs.Count) { throw 'No completed runs found.' }
$summary = @()
$querySummary = @()
foreach ($group in ($runs | Group-Object runtime)) {
    foreach ($phase in 'idle','ragdolls') {
        $values = @($group.Group | ForEach-Object { $_.phases | Where-Object phase -eq $phase })
        $summary += [pscustomobject]@{
            runtime = $group.Name; phase = $phase; runs = $values.Count
            mean_total_ms = Quantile @($values.mean_total_ms) .5
            p95_total_ms = Quantile @($values.p95_total_ms) .5
            total_min_run_ms = ($values.mean_total_ms | Measure-Object -Minimum).Minimum
            total_max_run_ms = ($values.mean_total_ms | Measure-Object -Maximum).Maximum
            mean_solver_ms = Quantile @($values.mean_solver_ms) .5
            p95_solver_ms = Quantile @($values.p95_solver_ms) .5
            mean_collision_ms = Quantile @($values.mean_collision_ms) .5
            mean_native_preparation_ms = Quantile @($values.mean_native_preparation_ms) .5
            mean_native_integration_ms = Quantile @($values.mean_native_integration_ms) .5
            mean_native_feedback_ms = Quantile @($values.mean_native_feedback_ms) .5
            median_peak_working_set_mib = Quantile @($group.Group.maximum_working_set_mib) .5
            save_reload_validated_runs = @($group.Group | Where-Object save_reload_validated).Count
            mean_bodies = Quantile @($values.mean_bodies) .5
            mean_contacts = Quantile @($values.mean_contacts) .5
            mean_joints = Quantile @($values.mean_joints) .5
            mean_frame_ms = Quantile @($values.mean_frame_ms) .5
            p95_frame_ms = Quantile @($values.p95_frame_ms) .5
            p99_frame_ms = Quantile @($values.p99_frame_ms) .5
        }
    }
    foreach ($workload in 'nearest_ray','any_ray','full_box') {
        $values = @($group.Group | ForEach-Object { $_.queries | Where-Object workload -eq $workload })
        $querySummary += [pscustomobject]@{
            runtime = $group.Name; workload = $workload; runs = $values.Count
            median_us_per_query = Quantile @($values.median_us_per_query) .5
            min_run_us_per_query = ($values.median_us_per_query | Measure-Object -Minimum).Minimum
            max_run_us_per_query = ($values.median_us_per_query | Measure-Object -Maximum).Maximum
            model_reported_bytes = $values[0].model_reported_bytes
        }
    }
}
$summary | Export-Csv -LiteralPath (Join-Path $OutputRoot 'physics-summary.csv') -NoTypeInformation
$querySummary | Export-Csv -LiteralPath (Join-Path $OutputRoot 'query-summary.csv') -NoTypeInformation
@{ runs = $runs; physics = $summary; queries = $querySummary } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $OutputRoot 'summary.json')
$summary | Format-Table runtime,phase,runs,mean_total_ms,mean_solver_ms,mean_bodies,mean_contacts,p95_frame_ms -AutoSize
$querySummary | Format-Table runtime,workload,runs,median_us_per_query -AutoSize
