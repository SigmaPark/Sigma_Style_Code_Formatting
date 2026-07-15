# generate_variants.ps1 -- mass-produce battery variants with the local LLM "yeon"
# (Ollama, deterministic). Pure ASCII, PowerShell 5.1.
#
# For each template in test/battery/cases/, asks yeon for N surface variants that
# rename identifiers and reword string literals while keeping every line's
# structure, line count, and // EXPECT headers exactly unchanged. Output goes to
# test/battery/gen/<case>__v<N>.cpp (gitignored). run_battery.ps1 validates each
# variant (line count + EXPECT set) and skips invalid generator output.
#
# Usage:  powershell -NoProfile -File test\battery\generate_variants.ps1 [-Variants 3]

param(
	[int]$Variants = 3,
	[string]$Model = "yeon",
	[string]$Api = "http://localhost:11434/api/generate"
)

$casesDir = Join-Path $PSScriptRoot "cases"
$genDir = Join-Path $PSScriptRoot "gen"

$tags = & ollama list 2>$null
if (-not ($tags -match "yeon")) {
	Write-Output "[gen] model 'yeon' not found in ollama list -- aborting"
	exit 2
}

New-Item -ItemType Directory -Force $genDir | Out-Null

$files = @(Get-ChildItem -Path $casesDir -Filter *.cpp | Sort-Object Name)
$made = 0
$bad = 0

foreach ($f in $files) {
	$code = [System.IO.File]::ReadAllText($f.FullName)
	for ($v = 1; $v -le $Variants; $v += 1) {
		$prompt = @"
You are generating test data for a C++ code-formatting checker.
Rewrite the sample below as VARIANT $v of $Variants, applying ONLY these changes:
- rename identifiers (functions, variables, types, macros) to different plausible names
- reword the contents of string literals (keep any leading/trailing spaces inside them)
Hard rules you must not break:
- never rename C++ keywords (if, else, do, while, try, catch, return, struct, class, new, ...)
- keep the exact same number of lines
- keep every line's indentation, whitespace, punctuation and operators exactly as-is
- copy every line that starts with // EXPECT completely unchanged
- output ONLY the resulting code: no explanations, no markdown fences

$code
"@
		$body = @{ model = $Model; prompt = $prompt; stream = $false; options = @{ temperature = 0 } } | ConvertTo-Json -Depth 4
		try {
			$r = Invoke-RestMethod -Uri $Api -Method Post -Body $body -ContentType 'application/json' -TimeoutSec 180
		} catch {
			Write-Output "[gen] request failed on $($f.Name) v$v -- $($_.Exception.Message)"
			$bad += 1
			continue
		}
		$text = "$($r.response)"
		# strip accidental markdown fences and leading/trailing blank lines
		$lines = $text -split "`r?`n"
		$lines = $lines | Where-Object { $_ -notmatch '^\s*```' }
		while ($lines.Count -gt 0 -and $lines[0] -match '^\s*$') { $lines = $lines[1..($lines.Count - 1)] }
		while ($lines.Count -gt 0 -and $lines[$lines.Count - 1] -match '^\s*$') { $lines = $lines[0..($lines.Count - 2)] }
		$outPath = Join-Path $genDir ($f.BaseName + "__v$v.cpp")
		[System.IO.File]::WriteAllText($outPath, (($lines -join "`n") + "`n"))
		$made += 1
		Write-Output "[gen] $($f.BaseName)__v$v.cpp"
	}
}

Write-Output "[gen] done: $made written, $bad failed"
