param(
    [Parameter(Mandatory=$true)][string]$BenchmarkExe,
    [Parameter(Mandatory=$true)][string]$ProfileExe,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [ValidateRange(1,1000)][int]$Iterations=10,
    [ValidateRange(1,101)][int]$Samples=9,
    [ValidateRange(1,9)][int]$Rounds=3,
    [ValidateRange(1,4096)][int]$Base=256,
    [ValidateRange(0,62)][int]$Cpu=0
)
$ErrorActionPreference='Stop'
if([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT){throw 'This runner requires Windows'}
$repoRoot=Split-Path -Parent $PSScriptRoot
$safeRepoRoot=$repoRoot.Replace('\','/')
function SourceHash([string]$path) {
    $text=[IO.File]::ReadAllText($path).Replace("`r`n","`n")
    $stream=[IO.MemoryStream]::new([Text.UTF8Encoding]::new($false).GetBytes($text))
    try { (Get-FileHash -InputStream $stream -Algorithm SHA256).Hash.ToLowerInvariant() }
    finally { $stream.Dispose() }
}
$benchmarkPath=(Resolve-Path -LiteralPath $BenchmarkExe).Path
$profilePath=(Resolve-Path -LiteralPath $ProfileExe).Path
$outPath=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $outPath){throw 'Choose a fresh output directory'}
New-Item -ItemType Directory -Path $outPath | Out-Null
$dataPath=Join-Path $outPath 'data'
$process=[Diagnostics.Process]::GetCurrentProcess()
$originalAffinity=$process.ProcessorAffinity
$mask=[Int64]1 -shl $Cpu
if(($originalAffinity.ToInt64() -band $mask) -eq 0){throw 'CPU is outside allowed affinity mask'}
try {
    $process.ProcessorAffinity=[IntPtr]$mask
    & $benchmarkPath --generate $dataPath > (Join-Path $outPath 'generate.log')
    if($LASTEXITCODE -ne 0){throw 'Legacy dataset generation failed'}
    & $benchmarkPath --generate-profile $dataPath $Base >> (Join-Path $outPath 'generate.log')
    if($LASTEXITCODE -ne 0){throw 'Profile dataset generation failed'}
    $inputs=@(Get-ChildItem -LiteralPath $dataPath -Filter '*.json' -File | Sort-Object Name)
    $inputPaths=@($inputs | ForEach-Object {$_.FullName})
    & $profilePath --self-test > (Join-Path $outPath 'self-test.log')
    if($LASTEXITCODE -ne 0){throw 'Tracker self-test failed'}
    & $profilePath @inputPaths > (Join-Path $outPath 'memory.csv') 2> (Join-Path $outPath 'memory.stderr')
    if($LASTEXITCODE -ne 0){throw 'Memory baseline failed'}
    Write-Output 'Memory baseline complete'
    for($round=1;$round -le $Rounds;++$round){
        $arguments=@('--stages','--iterations',"$Iterations",'--samples',"$Samples",'--csv',
            (Join-Path $outPath "timing-$round.csv"))+$inputPaths
        & $benchmarkPath @arguments > (Join-Path $outPath "timing-$round.log") 2> (Join-Path $outPath "timing-$round.stderr")
        if($LASTEXITCODE -ne 0){throw "Timing round $round failed"}
        Write-Output "Timing round $round/$Rounds complete"
    }
    $sources=@('benchmarks/ParseBenchmark.cpp','benchmarks/ParserProfile.cpp',
        'benchmarks/MemoryTracker.cpp','benchmarks/MemoryTracker.hpp','benchmarks/CMakeLists.txt',
        'benchmarks/BenchmarkUtils.cpp','benchmarks/BenchmarkUtils.hpp','benchmarks/RunMemoryBaseline.ps1')
    $revision=git -c "safe.directory=$safeRepoRoot" -C $repoRoot rev-parse HEAD
    if($LASTEXITCODE -ne 0){throw 'Unable to determine source revision'}
    $metadata=[ordered]@{
        base_revision=$revision.Trim()
        cpu=(Get-ItemProperty 'HKLM:\HARDWARE\DESCRIPTION\System\CentralProcessor\0' -Name ProcessorNameString).ProcessorNameString.Trim()
        os=[Environment]::OSVersion.VersionString
        logical_cpu=$Cpu; iterations=$Iterations; samples=$Samples; rounds=$Rounds; base=$Base
        benchmark_sha256=(Get-FileHash -LiteralPath $benchmarkPath -Algorithm SHA256).Hash.ToLowerInvariant()
        profile_sha256=(Get-FileHash -LiteralPath $profilePath -Algorithm SHA256).Hash.ToLowerInvariant()
        libraries=@(Import-Csv (Join-Path $outPath 'timing-1.csv') | Select-Object library,version,compiler,configuration -Unique)
        source_hash_format='UTF-8 without BOM, LF line endings'
        tool_sources=@($sources | ForEach-Object {[ordered]@{file=$_;sha256=(SourceHash (Join-Path $repoRoot $_))}})
        inputs=@($inputs | ForEach-Object {[ordered]@{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}})
    }
    [IO.File]::WriteAllText((Join-Path $outPath 'metadata.json'),($metadata | ConvertTo-Json -Depth 6)+"`n",[Text.UTF8Encoding]::new($false))
} finally {$process.ProcessorAffinity=$originalAffinity}
Write-Output "Saved baseline to $outPath"
