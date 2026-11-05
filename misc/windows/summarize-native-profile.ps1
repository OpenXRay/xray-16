param(
    [Parameter(Mandatory)][string]$Log,
    [Parameter(Mandatory)][string]$Output
)
$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
$samples = @()
$phase = $null
foreach ($line in Get-Content -LiteralPath $Log) {
    if ($line -match 'GAMEPLAY_BENCHMARK BEGIN phase=(\w+)') { $phase = $Matches[1]; continue }
    if ($line -match 'GAMEPLAY_BENCHMARK END phase=') { $phase = $null; continue }
    if (!$phase -or $line -notmatch '(NATIVE_(?:CORE|WORLD|WAKE|CHARACTER|MOTOR|CONTACT|PREPARATION)_PROFILE)\b') { continue }
    $marker = $Matches[1]
    foreach ($field in [regex]::Matches($line, '(\w+)=(-?\d+(?:\.\d+)?)(?:/(\d+))?')) {
        $name = $field.Groups[1].Value
        $value = [double]::Parse($field.Groups[2].Value, $culture)
        if ($field.Groups[3].Success) {
            $samples += [pscustomobject]@{ phase=$phase; marker=$marker; field="${name}_calls"; value=$value }
            $samples += [pscustomobject]@{ phase=$phase; marker=$marker; field="${name}_sleeping"; value=[double]::Parse($field.Groups[3].Value, $culture) }
        } else {
            $samples += [pscustomobject]@{ phase=$phase; marker=$marker; field=$name; value=$value }
        }
    }
}
if (!$samples.Count) { throw 'No native profile samples inside benchmark phases.' }
$summary = foreach ($group in $samples | Group-Object phase,marker,field) {
    # Averaged counters can straddle BEGIN. Discard the first sample of each
    # marker/field in each phase, and exclude all loading/reload/quit output.
    $rows = @($group.Group | Select-Object -Skip 1)
    if (!$rows.Count) { continue }
    $stats = $rows.value | Measure-Object -Average -Minimum -Maximum
    [pscustomobject]@{
        phase=$rows[0].phase; marker=$rows[0].marker; field=$rows[0].field
        samples=$rows.Count; mean=$stats.Average; minimum=$stats.Minimum; maximum=$stats.Maximum
    }
}
if (!$summary) { throw 'Not enough samples after excluding phase boundary windows.' }
$summary | Sort-Object phase,marker,field | Export-Csv -LiteralPath $Output -NoTypeInformation
