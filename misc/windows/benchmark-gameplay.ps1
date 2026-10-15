param(
    [Parameter(Mandatory)][string]$RuntimeManifest,
    [Parameter(Mandatory)][string]$TemplateRoot,
    [Parameter(Mandatory)][string]$GameFiles,
    [Parameter(Mandatory)][string]$OutputRoot,
    [string]$Save = 'collision-start',
    [ValidateRange(1, 20)][int]$Repetitions = 3
)
$ErrorActionPreference = 'Stop'
$sourceRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$OutputRoot = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath (Join-Path $OutputRoot 'results')) {
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
Add-Type -AssemblyName System.Windows.Forms
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class BenchmarkInput {
    [StructLayout(LayoutKind.Sequential)] public struct Rect { public int left, top, right, bottom; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out Rect rect);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint f, int x, int y, uint d, UIntPtr e);
    [DllImport("user32.dll")] public static extern void keybd_event(byte k, byte s, uint f, UIntPtr e);
    [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint c, uint t);
}
'@
function Key([byte]$code) {
    $scan = [byte][BenchmarkInput]::MapVirtualKey($code, 0)
    [BenchmarkInput]::keybd_event($code, $scan, 8, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 100
    [BenchmarkInput]::keybd_event($code, $scan, 10, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 500
}
function Setting([string]$text, [string]$key, [string]$value) {
    $pattern = '(?m)^' + [regex]::Escape($key) + '\s+[^\r\n]*'
    if ([regex]::IsMatch($text, $pattern)) { return [regex]::Replace($text, $pattern, "$key $value") }
    return $text + "`r`n$key $value`r`n"
}
$configuration = Get-Content -LiteralPath (Join-Path $TemplateRoot 'userdata/user.ltx') -Raw
foreach ($setting in @(@('mt_physics','off'), @('rs_always_active','on'), @('rs_v_sync','off'),
    @('rs_fps_limit','501'), @('rs_fullscreen','off'), @('vid_mode','800x600'), @('rs_stats','off'),
    @('ph_frequency','100.'), @('ph_iterations','18'))) {
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
Set-Content -LiteralPath $configFile -Value $configuration -Encoding ASCII
$machine.config_sha256 = (Get-FileHash -LiteralPath $configFile).Hash
$machine | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $OutputRoot 'machine.json')

for ($repetition = 1; $repetition -le $Repetitions; ++$repetition) {
    # Reverse each alternate pass to reduce a consistent warm-up/order bias.
    $order = @($runtimes)
    if ($repetition % 2 -eq 0) { [array]::Reverse($order) }
    foreach ($runtime in $order) {
        $root = Join-Path $OutputRoot "games/$($runtime.name)"
        $result = Join-Path $OutputRoot "results/$($runtime.name)/run-$repetition"
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
        $game = Start-Process -FilePath (Join-Path $runtime.directory 'xrEngine.exe') -WorkingDirectory $root -WindowStyle Hidden -PassThru -ArgumentList @(
            '-nosplash','-force_flushlog','-fsltx','fsgame.ltx',
            '-start',"server($Save/single/alife/load)",'client(localhost)')
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
            [BenchmarkInput]::ShowWindow($game.MainWindowHandle, 9) | Out-Null
            [BenchmarkInput]::SetForegroundWindow($game.MainWindowHandle) | Out-Null
            Start-Sleep -Seconds 1
            if ([BenchmarkInput]::GetForegroundWindow() -ne $game.MainWindowHandle) { throw 'Could not focus the game.' }
            $rect = New-Object BenchmarkInput+Rect
            [BenchmarkInput]::GetWindowRect($game.MainWindowHandle, [ref]$rect) | Out-Null
            [BenchmarkInput]::SetCursorPos(($rect.left + $rect.right) / 2, ($rect.top + $rect.bottom) / 2) | Out-Null
            [BenchmarkInput]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero)
            Start-Sleep -Milliseconds 100
            [BenchmarkInput]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
            Start-Sleep -Seconds 2
            Key 192
            [System.Windows.Forms.SendKeys]::SendWait('run_script gameplay_benchmark')
            Key 13
            Key 192
            $watch.Restart()
            $workingSet = 0L
            while (!$game.HasExited -and $watch.Elapsed.TotalSeconds -lt 120) {
                $game.Refresh()
                $workingSet = [Math]::Max($workingSet, $game.WorkingSet64)
                Start-Sleep -Milliseconds 500
            }
            if (!$game.HasExited) { throw 'Timed out running the benchmark.' }
            $contents = Get-Content -LiteralPath $log.FullName -Raw
            if ($contents -notmatch 'GAMEPLAY_BENCHMARK DONE' -or $contents -match 'FATAL ERROR|GAMEPLAY_BENCHMARK FAIL|SCRIPT RUNTIME ERROR') {
                throw 'Benchmark did not complete cleanly; inspect the saved log.'
            }
            foreach ($file in 'benchmark-idle.csv','benchmark-ragdolls.csv','benchmark-queries.csv') {
                Copy-Item -LiteralPath (Join-Path $root "userdata/$file") -Destination $result -Force
            }
            [ordered]@{
                runtime = $runtime.name; collision = $runtime.collision; dynamics = $runtime.dynamics
                repetition = $repetition; exit_code = $game.ExitCode
                maximum_observed_working_set_bytes = $workingSet
                physics_sha256 = (Get-FileHash -LiteralPath (Join-Path $runtime.directory 'xrPhysics.dll')).Hash
                collision_sha256 = (Get-FileHash -LiteralPath (Join-Path $runtime.directory 'xrCDB.dll')).Hash
                saved_at_utc = [DateTime]::UtcNow.ToString('o')
            } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'run.json')
            Write-Host "Completed $($runtime.name), repetition $repetition"
        }
        finally {
            if (!$game.HasExited) { Stop-Process -Id $game.Id }
            if ($log) { Copy-Item -LiteralPath $log.FullName -Destination (Join-Path $result 'engine.log') -Force }
        }
    }
}
