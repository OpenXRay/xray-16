param(
    [Parameter(Mandatory)][string]$RuntimeManifest,
    [Parameter(Mandatory)][string]$TemplateRoot,
    [Parameter(Mandatory)][string]$GameFiles,
    [Parameter(Mandatory)][string]$OutputRoot,
    [string]$Save = 'collision-start',
    [ValidateRange(1, 20)][int]$Repetitions = 3,
    [switch]$ValidateReload,
    [switch]$Resume
)
$ErrorActionPreference = 'Stop'
$sourceRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$OutputRoot = [IO.Path]::GetFullPath($OutputRoot)
if (!$Resume -and (Test-Path -LiteralPath (Join-Path $OutputRoot 'results'))) {
    throw 'Use a fresh OutputRoot so captures from different comparisons cannot be mixed.'
}
$runtimes = Get-Content -LiteralPath $RuntimeManifest -Raw | ConvertFrom-Json
if (@($runtimes.name | Select-Object -Unique).Count -ne @($runtimes).Count) { throw 'Runtime names must be unique.' }
if (Get-Process xrEngine -ErrorAction SilentlyContinue) { throw 'Close the existing game before benchmarking.' }
foreach ($runtime in $runtimes) {
    if ($runtime.name -notmatch '^[A-Za-z0-9_-]+$') { throw 'Invalid runtime name.' }
    foreach ($file in 'xrEngine.exe','xrCDB.dll','xrPhysics.dll') {
        if (!(Test-Path -LiteralPath (Join-Path $runtime.directory $file))) { throw "Missing $file in $($runtime.directory)" }
    }
}
$engineHashes = @($runtimes | ForEach-Object { (Get-FileHash -LiteralPath (Join-Path $_.directory 'xrEngine.exe')).Hash } | Select-Object -Unique)
if ($engineHashes.Count -ne 1) { throw 'Use the same engine executable for every backend.' }
New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null
function Setting([string]$text, [string]$key, [string]$value) {
    $pattern = '(?m)^' + [regex]::Escape($key) + '\s+[^\r\n]*'
    if ([regex]::IsMatch($text, $pattern)) { return [regex]::Replace($text, $pattern, "$key $value") }
    return $text + "`r`n$key $value`r`n"
}
$configuration = Get-Content -LiteralPath (Join-Path $TemplateRoot 'userdata/user.ltx') -Raw
foreach ($setting in @(@('mt_physics','off'), @('rs_always_active','on'), @('rs_v_sync','off'),
    @('rs_fps_limit','501'), @('rs_fullscreen','off'), @('vid_mode','800x600'), @('rs_stats','off'),
    @('ph_frequency','100.'), @('ph_iterations','18'), @('keypress_on_start','0'))) {
    $configuration = Setting $configuration $setting[0] $setting[1]
}
$machine = [ordered]@{
    cpu = @(Get-CimInstance Win32_Processor | Select-Object Name, NumberOfCores, NumberOfLogicalProcessors)
    gpu = @(Get-CimInstance Win32_VideoController | Select-Object Name, DriverVersion)
    os = [Environment]::OSVersion.VersionString
    source_commit = (& git -C $sourceRoot rev-parse HEAD)
    source_changes = @(& git -C $sourceRoot status --porcelain)
    config_sha256 = $null
    resolution = '800x600'
    repetitions = $Repetitions
    engine_sha256 = $engineHashes[0]
}
$configFile = Join-Path $OutputRoot 'benchmark-user.ltx'
if ($Resume -and (Test-Path -LiteralPath $configFile)) {
    if ((Get-Content -LiteralPath $configFile -Raw).TrimEnd() -ne $configuration.TrimEnd()) {
        throw 'Configuration changed since the interrupted comparison.'
    }
}
Set-Content -LiteralPath $configFile -Value $configuration -Encoding ASCII
$machine.config_sha256 = (Get-FileHash -LiteralPath $configFile).Hash
if (!$Resume -or !(Test-Path -LiteralPath (Join-Path $OutputRoot 'machine.json'))) {
    $machine | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $OutputRoot 'machine.json')
}

for ($repetition = 1; $repetition -le $Repetitions; ++$repetition) {
    # Reverse each alternate pass to reduce a consistent warm-up/order bias.
    $order = @($runtimes)
    if ($repetition % 2 -eq 0) { [array]::Reverse($order) }
    foreach ($runtime in $order) {
        $root = Join-Path $OutputRoot "games/$($runtime.name)"
        $result = Join-Path $OutputRoot "results/$($runtime.name)/run-$repetition"
        $completed = Join-Path $result 'run.json'
        if ($Resume -and (Test-Path -LiteralPath $completed)) {
            $previous = Get-Content -LiteralPath $completed -Raw | ConvertFrom-Json
            foreach ($file in @(@('xrPhysics.dll','physics_sha256'), @('xrCDB.dll','collision_sha256'), @('xrGame.dll','game_sha256'))) {
                if ($previous.($file[1]) -ne (Get-FileHash -LiteralPath (Join-Path $runtime.directory $file[0])).Hash) {
                    throw "Runtime changed since the completed capture: $($runtime.name) / $($file[0])"
                }
            }
            if ($previous.exit_code -ne 0 -or $previous.collision -ne $runtime.collision -or
                $previous.dynamics -ne $runtime.dynamics -or $previous.workers -ne $runtime.workers) {
                throw 'Completed capture disagrees with the runtime manifest.'
            }
            if ($ValidateReload -and !$previous.save_reload_validated) {
                throw 'The completed capture does not include save/reload validation.'
            }
            if ($previous.core_sha256 -and $previous.core_sha256 -ne
                (Get-FileHash -LiteralPath (Join-Path $runtime.directory 'xrPhysicsCore.dll')).Hash) {
                throw 'Native core changed since the completed capture.'
            }
            Write-Host "Keeping completed $($runtime.name), repetition $repetition"
            continue
        }
        New-Item -ItemType Directory -Path $root,$result,(Join-Path $root 'userdata/savedgames') -Force | Out-Null
        foreach ($directory in 'levels','localization','mp','patches','resources') {
            $junction = Join-Path $root $directory
            if (!(Test-Path -LiteralPath $junction)) {
                New-Item -ItemType Junction -Path $junction -Target (Join-Path $GameFiles $directory) | Out-Null
            }
        }
        Copy-Item -LiteralPath (Join-Path $TemplateRoot 'fsgame.ltx') -Destination $root -Force
        Copy-Item -LiteralPath (Join-Path $TemplateRoot 'gamedata') -Destination $root -Recurse -Force
        New-Item -ItemType Directory -Path (Join-Path $root 'gamedata/scripts') -Force | Out-Null
        Copy-Item -LiteralPath (Join-Path $sourceRoot 'src/xrPhysics/tests/gameplay_benchmark.script') -Destination (Join-Path $root 'gamedata/scripts') -Force
        Copy-Item -LiteralPath (Join-Path $TemplateRoot "userdata/savedgames/$Save.scop") -Destination (Join-Path $root 'userdata/savedgames') -Force
        Copy-Item -LiteralPath $configFile -Destination (Join-Path $root 'userdata/user.ltx') -Force
        foreach ($file in 'benchmark-idle.csv','benchmark-ragdolls.csv','benchmark-queries.csv') {
            $old = Join-Path $root "userdata/$file"
            if (Test-Path -LiteralPath $old) { Remove-Item -LiteralPath $old }
        }
        $logDir = Join-Path $root 'userdata/logs'
        $oldLogs = @(Get-ChildItem -LiteralPath $logDir -Filter '*.log' -ErrorAction SilentlyContinue)
        foreach ($old in $oldLogs) { Remove-Item -LiteralPath $old.FullName }
        Write-Host "Starting $($runtime.name), repetition $repetition"
        $previousWorkers = $env:XRAY_JOLT_WORKERS
        if ($null -ne $runtime.workers) {
            if ($runtime.workers -lt 0 -or $runtime.workers -gt 32) { throw 'Workers must be 0..32.' }
            $env:XRAY_JOLT_WORKERS = [string]$runtime.workers
        }
        $game = Start-Process -FilePath (Join-Path $runtime.directory 'xrEngine.exe') -WorkingDirectory $root -WindowStyle Hidden -PassThru -ArgumentList @(
            '-nosplash','-force_flushlog','-fsltx','fsgame.ltx',
            '-start',"server($Save/single/alife/load)",'client(localhost)','-$run_script','gameplay_benchmark')
        try {
            $watch = [Diagnostics.Stopwatch]::StartNew()
            $log = $null
            while ($watch.Elapsed.TotalSeconds -lt 150) {
                $game.Refresh()
                if ($game.HasExited) { throw 'Game exited during load.' }
                $log = Get-ChildItem -LiteralPath $logDir -Filter '*.log' -ErrorAction SilentlyContinue | Select-Object -First 1
                if ($log -and (Get-Content -LiteralPath $log.FullName -Raw) -match 'End of synchronization A\[1\] R\[1\]') { break }
                Start-Sleep -Milliseconds 500
            }
            if ($watch.Elapsed.TotalSeconds -ge 150) { throw 'Timed out loading the save.' }
            $watch.Restart()
            $workingSet = 0L
            while (!$game.HasExited -and $watch.Elapsed.TotalSeconds -lt 120) {
                $game.Refresh()
                $workingSet = [Math]::Max($workingSet, $game.WorkingSet64)
                if ((Get-Content -LiteralPath $log.FullName -Raw) -match
                    'FATAL ERROR|GAMEPLAY_BENCHMARK FAIL|NATIVE_GAMEPLAY_CHECK FAIL|SCRIPT RUNTIME ERROR') {
                    throw 'Benchmark failed; inspect the saved engine.log.'
                }
                Start-Sleep -Milliseconds 500
            }
            if (!$game.HasExited) { throw 'Timed out running the benchmark.' }
            $contents = Get-Content -LiteralPath $log.FullName -Raw
            if ($contents -notmatch 'GAMEPLAY_BENCHMARK DONE' -or $contents -match 'FATAL ERROR|GAMEPLAY_BENCHMARK FAIL|NATIVE_GAMEPLAY_CHECK FAIL|SCRIPT RUNTIME ERROR') {
                throw 'Benchmark did not complete cleanly; inspect the saved log.'
            }
            $benchmarkExitCode = $game.ExitCode
            if ($ValidateReload) {
                $saveFile = Join-Path $root 'userdata/savedgames/native-physics-validation.scop'
                if (!(Test-Path -LiteralPath $saveFile)) { throw 'Game did not write the validation save.' }
                $states = [regex]::Matches($contents, 'SAVE_OBJECT id=(\d+) elements=(\d+) joints=(\d+) mass=([\d.]+) x=([-\d.]+) y=([-\d.]+) z=([-\d.]+) bx=([-\d.]+) by=([-\d.]+) bz=([-\d.]+)')
                if ($states.Count -ne 12) { throw 'Missing saved ragdoll states.' }
                $expected = @('objects = {')
                foreach ($state in $states) {
                    $values = @($state.Groups | Select-Object -Skip 1 | ForEach-Object Value)
                    $expected += '{{id={0},elements={1},joints={2},mass={3},x={4},y={5},z={6},bx={7},by={8},bz={9}}},' -f $values
                }
                $expected += '}'
                Set-Content -LiteralPath (Join-Path $root 'gamedata/scripts/gameplay_reload_expected.script') -Value $expected -Encoding ASCII
                Copy-Item -LiteralPath (Join-Path $sourceRoot 'src/xrPhysics/tests/gameplay_reload.script') -Destination (Join-Path $root 'gamedata/scripts') -Force
                Copy-Item -LiteralPath $log.FullName -Destination (Join-Path $result 'engine.log') -Force
                Copy-Item -LiteralPath $saveFile -Destination $result -Force
                $reload = Start-Process -FilePath (Join-Path $runtime.directory 'xrEngine.exe') -WorkingDirectory $root -WindowStyle Hidden -PassThru -ArgumentList @(
                    '-nosplash','-force_flushlog','-fsltx','fsgame.ltx',
                    '-start','server(native-physics-validation/single/alife/load)','client(localhost)','-$run_script','gameplay_reload')
                try {
                    $watch.Restart()
                    while (!$reload.HasExited -and $watch.Elapsed.TotalSeconds -lt 150) {
                        $reload.Refresh()
                        if ((Get-Content -LiteralPath $log.FullName -Raw) -match 'FATAL ERROR|GAMEPLAY_RELOAD FAIL|SCRIPT RUNTIME ERROR') {
                            throw 'Save/reload failed; inspect reload.log.'
                        }
                        Start-Sleep -Milliseconds 500
                    }
                    if (!$reload.HasExited) { throw 'Timed out reloading validation save.' }
                    if ($reload.ExitCode -ne 0 -or (Get-Content -LiteralPath $log.FullName -Raw) -notmatch 'GAMEPLAY_RELOAD DONE') {
                        throw 'Validation reload did not complete cleanly.'
                    }
                } finally {
                    if (!$reload.HasExited) { Stop-Process -Id $reload.Id; $reload.WaitForExit(10000) | Out-Null }
                    Copy-Item -LiteralPath $log.FullName -Destination (Join-Path $result 'reload.log') -Force
                }
            }
            foreach ($file in 'benchmark-idle.csv','benchmark-ragdolls.csv','benchmark-queries.csv') {
                Copy-Item -LiteralPath (Join-Path $root "userdata/$file") -Destination $result -Force
            }
            [ordered]@{
                runtime = $runtime.name; collision = $runtime.collision; dynamics = $runtime.dynamics
                workers = $runtime.workers
                repetition = $repetition; exit_code = $benchmarkExitCode
                save_reload_validated = [bool]$ValidateReload
                maximum_observed_working_set_bytes = $workingSet
                physics_sha256 = (Get-FileHash -LiteralPath (Join-Path $runtime.directory 'xrPhysics.dll')).Hash
                collision_sha256 = (Get-FileHash -LiteralPath (Join-Path $runtime.directory 'xrCDB.dll')).Hash
                game_sha256 = (Get-FileHash -LiteralPath (Join-Path $runtime.directory 'xrGame.dll')).Hash
                core_sha256 = $(if (Test-Path -LiteralPath (Join-Path $runtime.directory 'xrPhysicsCore.dll')) {
                    (Get-FileHash -LiteralPath (Join-Path $runtime.directory 'xrPhysicsCore.dll')).Hash
                } else { $null })
                saved_at_utc = [DateTime]::UtcNow.ToString('o')
            } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'run.json')
            Write-Host "Completed $($runtime.name), repetition $repetition"
        }
        finally {
            $env:XRAY_JOLT_WORKERS = $previousWorkers
            if (!$game.HasExited) { Stop-Process -Id $game.Id; $game.WaitForExit(10000) | Out-Null }
            if ($log -and !($ValidateReload -and (Test-Path -LiteralPath (Join-Path $result 'engine.log')))) {
                Copy-Item -LiteralPath $log.FullName -Destination (Join-Path $result 'engine.log') -Force
            }
        }
    }
}
