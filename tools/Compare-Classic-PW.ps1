# Compare stock Classic and PW Edition from the same Windows PC.
# External, read-only process measurements. No media/install changes.
[CmdletBinding()]
param([ValidateRange(15,600)][int]$Seconds=60)
$ErrorActionPreference='Stop'
$out=Join-Path $env:LOCALAPPDATA ('Pixelwolf\GGClassicPW\Comparison-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $out -Force | Out-Null
function Select-GameProcess {
    do {
        $candidates=@(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -match '^(GameGuru|Guru-MapEditor|Guru-Game|GameGuruClassic)$' } | Sort-Object Id)
        if(!$candidates.Count){ Write-Host 'No known GameGuru process detected. Launch the editor and load the test level.' }
        else { $candidates | Select-Object Id,ProcessName,MainWindowTitle | Format-Table -AutoSize | Out-Host }
        $answer=Read-Host 'Enter the PID of the actual running game/editor (Enter to refresh, Q to quit)'
        if($answer -eq 'Q'){throw 'Cancelled by operator'}
        $pidValue=0
        if([int]::TryParse($answer,[ref]$pidValue)){
            $selected=$candidates | Where-Object Id -eq $pidValue | Select-Object -First 1
            if($selected){return $selected}
        }
    } while($true)
}
function Read-GpuMemory([int]$processId){
    # CIM performance data is language-neutral; may be absent on older WDDM.
    try{
        $all=@(Get-CimInstance -ClassName Win32_PerfFormattedData_GPUPerformanceCounters_GPUProcessMemory -ErrorAction Stop)
        $mine=@($all | Where-Object { $_.Name -match ('^pid_'+$processId+'_') })
        if(!$mine.Count){return $null}
        $dedicated=($mine | Measure-Object -Property DedicatedUsage -Sum).Sum
        $shared=($mine | Measure-Object -Property SharedUsage -Sum).Sum
        return [pscustomobject]@{Dedicated=[double]$dedicated;Shared=[double]$shared}
    }catch{return $null}
}
function Capture-Run([string]$label){
    Write-Host ""
    Write-Host "=== $label ==="
    Write-Host 'Load the SAME unmodified test map, enter Test Level, use identical resolution and settings.'
    Write-Host 'Repeat the same route or leave the camera at the same position. Close other game instances.'
    $p=Select-GameProcess
    $processId=$p.Id
    $startTime=$p.StartTime
    $exePath=$p.Path
    $hash=$null
    if($exePath){try{$hash=(Get-FileHash -LiteralPath $exePath -Algorithm SHA256).Hash}catch{}}
    Read-Host "Press Enter when gameplay is ready. Capture starts in 5 seconds" | Out-Null
    Start-Sleep -Seconds 5
    $clock=[System.Diagnostics.Stopwatch]::StartNew()
    $samples=New-Object 'System.Collections.Generic.List[object]'
    $previousCpu=$null
    $previousTime=$null
    while($clock.Elapsed.TotalSeconds -lt $Seconds){
        try{$p=Get-Process -Id $processId -ErrorAction Stop;if($p.StartTime -ne $startTime){break}}catch{break}
        $elapsed=$clock.Elapsed.TotalSeconds
        $cpu=$p.TotalProcessorTime.TotalSeconds
        $cpuPercent=$null
        if($null -ne $previousCpu -and $elapsed -gt $previousTime){
            $cpuPercent=100*($cpu-$previousCpu)/($elapsed-$previousTime)/[Environment]::ProcessorCount
        }
        $gpu=Read-GpuMemory $processId
        $samples.Add([pscustomobject]@{seconds=[math]::Round($elapsed,3);cpu_percent=$cpuPercent;working_set_mib=$p.WorkingSet64/1MB;private_mib=$p.PrivateMemorySize64/1MB;gpu_valid=($null -ne $gpu);dedicated_gpu_mib=$(if($gpu){$gpu.Dedicated/1MB});shared_gpu_mib=$(if($gpu){$gpu.Shared/1MB})})
        $previousCpu=$cpu;$previousTime=$elapsed
        Start-Sleep -Milliseconds 1000
    }
    if($samples.Count -lt 2){throw "Insufficient measurements for $label"}
    $samples | Export-Csv -NoTypeInformation -Encoding UTF8 (Join-Path $out "$label.csv")
    $complete=$clock.Elapsed.TotalSeconds -ge $Seconds
    Write-Host 'Capture complete. Use the normal exit command now; do not force-close unless necessary.'
    Read-Host 'Press Enter immediately before attempting to exit the editor/game' | Out-Null
    $exitClock=[System.Diagnostics.Stopwatch]::StartNew()
    while($exitClock.Elapsed.TotalSeconds -lt 30){
        try{$check=Get-Process -Id $processId -ErrorAction Stop;if($check.StartTime -ne $startTime){break}}catch{break}
        Start-Sleep -Milliseconds 200
    }
    $exited=$false
    try{$check=Get-Process -Id $processId -ErrorAction Stop;$exited=$check.StartTime -ne $startTime}catch{$exited=$true}
    $gpuRows=@($samples | Where-Object gpu_valid)
    $validCpu=@($samples | Where-Object {$null -ne $_.cpu_percent})
    [pscustomobject]@{
        mode=$label;pid=$processId;exe=$exePath;sha256=$hash;complete=$complete;sample_count=$samples.Count;capture_seconds=$clock.Elapsed.TotalSeconds-$exitClock.Elapsed.TotalSeconds
        mean_private_mib=($samples|Measure-Object private_mib -Average).Average;peak_private_mib=($samples|Measure-Object private_mib -Maximum).Maximum
        mean_working_set_mib=($samples|Measure-Object working_set_mib -Average).Average
        mean_cpu_percent=$(if($validCpu.Count){($validCpu|Measure-Object cpu_percent -Average).Average})
        gpu_samples=$gpuRows.Count;mean_dedicated_gpu_mib=$(if($gpuRows.Count){($gpuRows|Measure-Object dedicated_gpu_mib -Average).Average})
        peak_dedicated_gpu_mib=$(if($gpuRows.Count){($gpuRows|Measure-Object dedicated_gpu_mib -Maximum).Maximum})
        mean_shared_gpu_mib=$(if($gpuRows.Count){($gpuRows|Measure-Object shared_gpu_mib -Average).Average})
        process_exit_observed=$exited;manual_exit_elapsed_seconds=$(if($exited){[math]::Round($exitClock.Elapsed.TotalSeconds,2)})
    }
}
Write-Host 'Stock Classic vs PW Edition — external process comparison'
Write-Host 'Use a verified clean stock installation for baseline; a modified shared Steam overlay is not stock.'
Write-Host 'This captures process CPU/RAM/GPU memory only, not FPS or render timings.'
Write-Host 'Run each configuration separately; confirm the actual game PID, not Steam or the launcher.'
$results=@()
foreach($mode in @('original-classic','pw-default','pw-streaming')){
    Write-Host "Prepare $mode with the intended flags and confirmed hardware support before continuing."
    $results+=Capture-Run $mode
    $results | ConvertTo-Json -Depth 5 | Set-Content -Encoding UTF8 (Join-Path $out 'summary.json')
}
$results | Export-Csv -NoTypeInformation -Encoding UTF8 (Join-Path $out 'summary.csv')
$results | Format-Table mode,complete,mean_private_mib,mean_dedicated_gpu_mib,mean_cpu_percent,process_exit_observed -AutoSize
if(@($results | Where-Object {!$_.complete -or !$_.gpu_samples}).Count){Write-Warning 'Incomplete capture or missing GPU data; do not infer savings.'}
if($results[0].sha256 -and $results[0].sha256 -eq $results[1].sha256){Write-Warning 'Original and PW selected executable hashes are identical: verify the baseline.'}
Write-Host 'Repeat three matched runs. This is descriptive evidence, not automatic acceptance.'
Write-Host "Results: $out"
