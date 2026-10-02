param(
    [Parameter(Mandatory)][string]$CopQueries,
    [Parameter(Mandatory)][string]$CsQueries,
    [Parameter(Mandatory)][string]$SyntheticRuns,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
function Number($value) { [double]::Parse($value, $culture) }
function Median($values) {
    $sorted = @($values | Sort-Object)
    if (!$sorted.Count) { throw 'No samples.' }
    $middle = [int][Math]::Floor($sorted.Count / 2)
    if ($sorted.Count % 2) { return $sorted[$middle] }
    ($sorted[$middle - 1] + $sorted[$middle]) / 2
}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$queryRuns = @()
foreach ($game in @(@{name='cop';root=$CopQueries},@{name='cs';root=$CsQueries})) {
    $inputSignature = $null
    $captures = @(Get-ChildItem -LiteralPath "$($game.root)/results" -Filter run.json -Recurse)
    if ($captures.Count -ne 12) { throw "Expected 12 accepted captures for $($game.name)." }
    foreach ($metadata in $captures) {
        $run = Get-Content -LiteralPath $metadata.FullName -Raw | ConvertFrom-Json
        $log = Get-Content -LiteralPath (Join-Path $metadata.DirectoryName 'engine.log') -Raw
        if ($run.exit_code -ne 0 -or $log -notmatch 'GAMEPLAY_BENCHMARK DONE' -or
            $log -match 'FATAL ERROR|SCRIPT RUNTIME ERROR|BENCHMARK FAIL') { throw 'Invalid query capture.' }
        $signature = "$($run.starting_save_sha256),$($run.script_sha256),$($run.system_sha256)"
        if ($inputSignature -and $signature -ne $inputSignature) { throw 'Query inputs differ.' }
        $inputSignature = $signature
        $rows = @(Import-Csv -LiteralPath (Join-Path $metadata.DirectoryName 'benchmark-queries.csv'))
        if ($rows.Count -ne 21) { throw 'Expected seven batches for each of three workloads.' }
        foreach ($workload in @('nearest_ray','any_ray','full_box')) {
            $samples = @($rows | Where-Object workload -eq $workload)
            if ($samples.Count -ne 7 -or (@($samples.iteration | Sort-Object) -join ',') -ne '0,1,2,3,4,5,6') {
                throw 'Missing query iteration.'
            }
            foreach ($field in @('queries','hits','range_sum','model_bytes','origin_x','origin_y','origin_z')) {
                if (@($samples | Select-Object -ExpandProperty $field -Unique).Count -ne 1) {
                    throw "Unstable query signature: $field."
                }
            }
            if (@($samples | Where-Object {
                $_.queries -ne '4096' -or $_.collision_backend -ne $run.collision -or $_.dynamics_backend -ne $run.dynamics
            }).Count) { throw 'Incorrect query count/backend.' }
            $queryRuns += [pscustomobject]@{
                game=$game.name; runtime=$run.runtime; repetition=$run.repetition; workload=$workload
                median_us_per_query=(Median @($samples | ForEach-Object { Number $_.total_ms })) * 1000 / 4096
                hits=[long]$samples[0].hits; range_sum=Number $samples[0].range_sum
                model_bytes=[long]$samples[0].model_bytes
                origin_x=Number $samples[0].origin_x; origin_y=Number $samples[0].origin_y; origin_z=Number $samples[0].origin_z
                collision_sha256=$run.collision_sha256; script_sha256=$run.script_sha256
                starting_save_sha256=$run.starting_save_sha256
            }
        }
    }
}
$querySummary = @()
foreach ($group in $queryRuns | Group-Object game,runtime,workload) {
    $rows = $group.Group
    if ($rows.Count -ne 3) { throw 'Expected three query captures per runtime/workload.' }
    foreach ($field in @('hits','range_sum','origin_x','origin_y','origin_z','collision_sha256')) {
        if (@($rows | Select-Object -ExpandProperty $field -Unique).Count -ne 1) { throw 'Query runs differ.' }
    }
    $reference = @($queryRuns | Where-Object { $_.game -eq $rows[0].game -and $_.runtime -eq 'ODE' -and $_.workload -eq $rows[0].workload })
    $sameOrigin = $reference[0].origin_x -eq $rows[0].origin_x -and $reference[0].origin_y -eq $rows[0].origin_y -and $reference[0].origin_z -eq $rows[0].origin_z
    if (!$sameOrigin) { throw 'Query origins differ between runtimes.' }
    $hitsMatch = $reference[0].hits -eq $rows[0].hits
    $rangeDifference = $rows[0].range_sum - $reference[0].range_sum
    if ($rows[0].runtime -eq 'Opcode13' -and (!$hitsMatch -or [Math]::Abs($rangeDifference) -gt 0.01)) {
        throw 'OPCODE 1.3 differs from the 1.2 query signature.'
    }
    $querySummary += [pscustomobject]@{
        game=$rows[0].game; runtime=$rows[0].runtime; workload=$rows[0].workload; runs=$rows.Count
        median_us_per_query=Median @($rows.median_us_per_query)
        min_us_per_query=($rows.median_us_per_query | Measure-Object -Minimum).Minimum
        max_us_per_query=($rows.median_us_per_query | Measure-Object -Maximum).Maximum
        change_percent=100 * ((Median @($rows.median_us_per_query)) / (Median @($reference.median_us_per_query)) - 1)
        hits=$rows[0].hits; hits_match_opcode12=$hitsMatch; range_sum=$rows[0].range_sum
        range_sum_difference=$rangeDifference; model_bytes=$rows[0].model_bytes
    }
}
$synthetic = @(Import-Csv -LiteralPath $SyntheticRuns)
if ($synthetic.Count -ne 12 -or @($synthetic | Where-Object exit_code -ne '0').Count) { throw 'Invalid synthetic captures.' }
if (@($synthetic.ray_sum | Select-Object -Unique).Count -ne 1 -or @($synthetic.box_hits | Select-Object -Unique).Count -ne 1) {
    throw 'Synthetic result signatures differ.'
}
$syntheticSummary = @()
foreach ($group in $synthetic | Group-Object runtime) {
    if ($group.Count -ne 3) { throw 'Expected three synthetic captures per runtime.' }
    $row = [ordered]@{runtime=$group.Name;runs=$group.Count}
    foreach ($metric in @('build_ms','ray_ms','box_ms','cache_ms')) {
        $values = @($group.Group | ForEach-Object { Number $_.$metric })
        $row[$metric] = Median $values
        $row["min_$metric"] = ($values | Measure-Object -Minimum).Minimum
        $row["max_$metric"] = ($values | Measure-Object -Maximum).Maximum
    }
    $row.model_bytes = [long]$group.Group[0].model_bytes
    $row.cache_bytes = [long]$group.Group[0].cache_bytes
    $syntheticSummary += [pscustomobject]$row
}
$queryRuns | Export-Csv -LiteralPath "$OutputDirectory/query-runs.csv" -NoTypeInformation
$querySummary | Export-Csv -LiteralPath "$OutputDirectory/query-summary.csv" -NoTypeInformation
$syntheticSummary | Export-Csv -LiteralPath "$OutputDirectory/synthetic-summary.csv" -NoTypeInformation
@{queries=$querySummary;synthetic=$syntheticSummary} | ConvertTo-Json -Depth 8 |
    Set-Content -LiteralPath "$OutputDirectory/collision-summary.json"
$querySummary | Format-Table game,runtime,workload,median_us_per_query,change_percent,hits_match_opcode12 -AutoSize
