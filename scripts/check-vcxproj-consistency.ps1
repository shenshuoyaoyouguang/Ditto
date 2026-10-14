<#
.SYNOPSIS
  Ensures every src/*.cpp and src/*.h is listed in CP_Main.vcxproj.

.DESCRIPTION
  MSBuild does not glob: a new file on disk is silently skipped by the build
  unless it has an explicit <ClCompile>/<ClInclude> entry in CP_Main.vcxproj
  (see AGENTS.md Gotchas). This script fails (exit 1) and lists offenders when
  a top-level src/*.cpp lacks a ClCompile entry or a src/*.h lacks a
  ClInclude entry. Project entries that point at a now-deleted src\<file>
  are reported as warnings only.

  Read-only: never modifies the vcxproj. Runs on Windows PowerShell 5.1 and
  PowerShell 7 (no v6+ only syntax; regex parse instead of the MSBuild XML
  namespace so both engines behave identically).
#>
param(
  [string]$RepoRoot = (Split-Path $PSScriptRoot -Parent),
  [string]$ProjectFile = (Join-Path $RepoRoot 'CP_Main.vcxproj'),
  [string]$SourceDir = (Join-Path $RepoRoot 'src')
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $ProjectFile)) {
  Write-Output ("::error file={0}::project file not found: {1}" -f $ProjectFile, $ProjectFile)
  exit 1
}
if (-not (Test-Path -LiteralPath $SourceDir)) {
  Write-Output ("::error::source dir not found: {0}" -f $SourceDir)
  exit 1
}

$text = [System.IO.File]::ReadAllText($ProjectFile)
$entries = New-Object 'System.Collections.Generic.HashSet[string]' (
  [System.StringComparer]::OrdinalIgnoreCase)
$pattern = '(?m)^\s*<(ClCompile|ClInclude)\s+Include\s*=\s*"([^"]+)"'
foreach ($m in [regex]::Matches($text, $pattern)) {
  $entries.Add($m.Groups[2].Value.Replace('/', '\')) | Out-Null
}

$failures = @()
foreach ($kind in @(@{ Ext = '.cpp'; Label = 'ClCompile' }, @{ Ext = '.h'; Label = 'ClInclude' })) {
  $files = Get-ChildItem -LiteralPath $SourceDir -Filter ('*' + $kind.Ext) -File
  foreach ($f in $files) {
    $expected = 'src\' + $f.Name
    if (-not $entries.Contains($expected)) {
      $failures += ('{0} {1} has no <{2}> entry in CP_Main.vcxproj' -f $kind.Ext, $f.Name, $kind.Label)
      Write-Output ("::error file=src/{0}::missing <{1}> entry for src/{0} in CP_Main.vcxproj" -f $f.Name, $kind.Label)
    }
  }
}

# Stale entries (project lists a top-level src\<file> that no longer exists):
# warning only -- deleting entries is a separate, deliberate change.
foreach ($e in $entries) {
  if ($e -match '^src\\[^\\]+$') {
    $diskPath = Join-Path $RepoRoot $e
    if (-not (Test-Path -LiteralPath $diskPath)) {
      Write-Output ("WARNING: stale entry {0} in CP_Main.vcxproj (file not on disk)" -f $e)
    }
  }
}

if ($failures.Count -gt 0) {
  Write-Output ('vcxproj consistency FAILED: {0} file(s) missing from CP_Main.vcxproj:' -f $failures.Count)
  foreach ($f in $failures) { Write-Output ('  MISSING: {0}' -f $f) }
  exit 1
}

Write-Output 'vcxproj consistency OK.'
exit 0
