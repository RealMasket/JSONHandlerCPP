param(
    [Parameter(Mandatory=$true)][string]$BeforeExe,
    [Parameter(Mandatory=$true)][string]$AfterExe,
    [Parameter(Mandatory=$true)][string[]]$Inputs,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [string]$BeforeRevision = "unspecified",
    [string]$AfterRevision = "working-tree",
    [ValidateRange(1,9)][int]$Rounds = 3,
    [ValidateRange(1,1000000)][int]$Iterations = 20,
    [ValidateRange(1,101)][int]$Samples = 9,
    [ValidateRange(0,62)][int]$Cpu = 0
)
$ErrorActionPreference = "Stop"
if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT) {
    throw "This affinity-controlled runner requires Windows."
}
$beforePath = (Resolve-Path -LiteralPath $BeforeExe).Path
$afterPath = (Resolve-Path -LiteralPath $AfterExe).Path
$inputPaths = @($Inputs | ForEach-Object { (Resolve-Path -LiteralPath $_).Path })
$outPath = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath (Join-Path $outPath "metadata.json")) {
    throw "Choose a fresh output directory to preserve complete prior measurements."
}
New-Item -ItemType Directory -Force -Path $outPath | Out-Null
$logPath = Join-Path $outPath "logs"
New-Item -ItemType Directory -Force -Path $logPath | Out-Null
$taskProcess = [Diagnostics.Process]::GetCurrentProcess()
$originalAffinity = $taskProcess.ProcessorAffinity
$mask = [Int64]1 -shl $Cpu
if (($originalAffinity.ToInt64() -band $mask) -eq 0) {
    throw "Requested CPU is outside the process's allowed affinity mask."
}
$cpuName = (Get-ItemProperty 'HKLM:\HARDWARE\DESCRIPTION\System\CentralProcessor\0' -Name ProcessorNameString).ProcessorNameString
$metadata = [ordered]@{
    before_revision = $BeforeRevision
    after_revision = $AfterRevision
    before_binary_sha256 = (Get-FileHash -LiteralPath $beforePath -Algorithm SHA256).Hash
    after_binary_sha256 = (Get-FileHash -LiteralPath $afterPath -Algorithm SHA256).Hash
    cpu = $cpuName.Trim()
    logical_cpu = $Cpu
    os = [Environment]::OSVersion.VersionString
    rounds = $Rounds
    iterations = $Iterations
    samples = $Samples
    inputs = @($inputPaths | ForEach-Object {
        [ordered]@{ name = [IO.Path]::GetFileName($_); bytes = (Get-Item -LiteralPath $_).Length; sha256 = (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash }
    })
}
$metadata | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $outPath "metadata.json") -Encoding utf8
try {
    # Child processes inherit this affinity. Set it before launch so even tiny
    # datasets run on the same CPU, without races against post-launch pinning.
    $taskProcess.ProcessorAffinity = [IntPtr]$mask
    for ($round = 1; $round -le $Rounds; ++$round) {
        $order = if ($round % 2) { @("before", "after") } else { @("after", "before") }
        foreach ($variant in $order) {
            $exe = if ($variant -eq "before") { $beforePath } else { $afterPath }
            $csv = Join-Path $outPath "$variant-$round.csv"
            $nativeArgs = @("--iterations", "$Iterations", "--samples", "$Samples", "--csv", $csv) + $inputPaths
            Write-Output "Round $round/$Rounds : $variant"
            & $exe @nativeArgs > (Join-Path $logPath "$variant-$round.log") 2> (Join-Path $logPath "$variant-$round.stderr")
            if ($LASTEXITCODE -ne 0) { throw "$variant benchmark failed with exit code $LASTEXITCODE" }
        }
    }
} finally {
    $taskProcess.ProcessorAffinity = $originalAffinity
}
Write-Output "Saved affinity-controlled comparison to $outPath"
