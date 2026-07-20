# run_battery.ps1 -- sample battery runner for sak (PowerShell 5.1, pure ASCII).
#
# Each case file declares its expected findings in trailing comment lines:
#     // EXPECT <row>:<tag>            e.g.  // EXPECT 5:9.3
#     // EXPECT-SUSPECT <row>:<tag>    e.g.  // EXPECT-SUSPECT 3:9.3
# A file with no EXPECT lines asserts that sak stays silent on it.
# The comparison is exact both ways: missing findings AND extra findings fail.
#
# Generated variants (test/battery/gen/, produced by generate_variants.ps1) are
# validated first: a variant whose line count differs from its template, or whose
# EXPECT header set differs, is skipped as invalid generator output.
#
# Usage:  powershell -NoProfile -File test\run_battery.ps1 [-Sak <path\to\sak.exe>]

param(
	[string]$Sak = "",
	[string]$CasesDir = "",
	[string]$GenDir = ""
)

$root = Split-Path -Parent $PSScriptRoot
if ($Sak -eq "") { $Sak = Join-Path $root "build\sak.exe" }
if ($CasesDir -eq "") { $CasesDir = Join-Path $PSScriptRoot "battery\cases" }
if ($GenDir -eq "") { $GenDir = Join-Path $PSScriptRoot "battery\gen" }

if (-not (Test-Path $Sak)) {
	Write-Output "[battery] sak not found: $Sak (build first: cmake --build build)"
	exit 2
}

function Get-Expects([string]$path) {
	$want = @()
	$wantS = @()
	foreach ($line in Get-Content $path) {
		if ($line -match '^//\s*EXPECT\s+(\d+):([0-9.]+)\s*$') {
			$want += "$($Matches[1]):$($Matches[2])"
		} elseif ($line -match '^//\s*EXPECT-SUSPECT\s+(\d+):([0-9.]+)\s*$') {
			$wantS += "$($Matches[1]):$($Matches[2])"
		}
	}
	return @{ v = ($want | Sort-Object); s = ($wantS | Sort-Object) }
}

function Get-Findings([string]$path) {
	$got = @()
	$gotS = @()
	$out = & $Sak $path 2>&1
	foreach ($line in $out) {
		$s = "$line"
		if ($s -match '^(.*):(\d+):(\d+) \[([^\]]+)\] (.*)$') {
			$key = "$($Matches[2]):$($Matches[4])"
			if ($Matches[5] -match '\[suspect\]\s*$') { $gotS += $key } else { $got += $key }
		}
	}
	return @{ v = ($got | Sort-Object); s = ($gotS | Sort-Object) }
}

function Join-Set($a) { if ($null -eq $a -or $a.Count -eq 0) { return "(none)" } return ($a -join ", ") }

$fails = 0
$runs = 0
$skips = 0

$files = @(Get-ChildItem -Path $CasesDir -Filter *.cpp | Sort-Object Name)
$genFiles = @()
if (Test-Path $GenDir) {
	$genFiles = @(Get-ChildItem -Path $GenDir -Filter *.cpp | Sort-Object Name)
}

foreach ($f in $genFiles) {
	# validate generated variant against its template: <template>__v<N>.cpp
	if ($f.BaseName -match '^(.+)__v\d+$') {
		$tmpl = Join-Path $CasesDir ($Matches[1] + ".cpp")
		if (Test-Path $tmpl) {
			$tLines = @(Get-Content $tmpl).Count
			$gLines = @(Get-Content $f.FullName).Count
			$tExp = Get-Expects $tmpl
			$gExp = Get-Expects $f.FullName
			$sameExp = ((Join-Set $tExp.v) -eq (Join-Set $gExp.v)) -and ((Join-Set $tExp.s) -eq (Join-Set $gExp.s))
			if ($tLines -ne $gLines -or -not $sameExp) {
				Write-Output "SKIP  $($f.Name) (invalid generator output: line count or EXPECT drifted)"
				$skips += 1
				continue
			}
		}
	}
	$files += $f
}

foreach ($f in $files) {
	$runs += 1
	$want = Get-Expects $f.FullName
	$got = Get-Findings $f.FullName
	$okV = (Join-Set $want.v) -eq (Join-Set $got.v)
	$okS = (Join-Set $want.s) -eq (Join-Set $got.s)
	if ($okV -and $okS) {
		Write-Output "PASS  $($f.Name)"
	} else {
		$fails += 1
		Write-Output "FAIL  $($f.Name)"
		if (-not $okV) {
			Write-Output "      want: $(Join-Set $want.v)"
			Write-Output "      got : $(Join-Set $got.v)"
		}
		if (-not $okS) {
			Write-Output "      want suspects: $(Join-Set $want.s)"
			Write-Output "      got  suspects: $(Join-Set $got.s)"
		}
	}
}

Write-Output ""
Write-Output "[battery] $runs run, $fails failed, $skips skipped"
if ($fails -gt 0) { exit 1 }
exit 0
