param([switch]$Preflight)

$ErrorActionPreference = 'Stop'
$env:Path = 'C:\mingw64\bin;' + $env:Path
$simulator = Join-Path $PSScriptRoot 'simulator.exe'
$strategies = 'hibbard_succ', 'hibbard_rand', 'scapegoat_inplace'
$seed = 20261117
$n = 4000
$pairs = 16000000
$sample = 4000

if ($Preflight) {
    $n = 32
    $pairs = 64
    $sample = 32
    $output = Join-Path $PSScriptRoot 'preflight'
} else {
    $output = Join-Path $PSScriptRoot ('run-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
}
New-Item -ItemType Directory -Path $output -Force | Out-Null

$results = foreach ($strategy in $strategies) {
    Write-Host "Running $strategy for $pairs pairs..."
    $csv = Join-Path $output "$strategy.csv"
    & $simulator --strategy $strategy --schedule uniform --n $n --seed $seed --pairs $pairs --sample $sample --out $csv
    if ($LASTEXITCODE -ne 0) {
        throw "Simulator failed for $strategy with exit code $LASTEXITCODE."
    }
    $rows = @(Import-Csv -LiteralPath $csv)
    $last = $rows[-1]
    if ($rows.Count -ne (1 + [Math]::Ceiling($pairs / $sample)) -or
        $last.pair_index -ne "$pairs" -or $last.strategy -ne $strategy) {
        throw "Unexpected CSV output for $strategy."
    }
    $last
}
$results | Format-Table strategy,n,seed,pair_index,avg_depth,rebuild_count -AutoSize
Write-Host "CSV files: $output"

