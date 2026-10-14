<#
.SYNOPSIS
  Guards resource.h against reused IDs and regressed _APS_NEXT_* markers.

.DESCRIPTION
  resource.h shares one ID space with CP_Main.rc (see AGENTS.md Gotchas), so
  a reused numeric value silently wires two symbols to one resource/command.

  Check 1 -- command-ID reuse: MFC dialog/control IDs are routinely shared
  across different dialogs (the current file has ~150 such intentional
  groups), so only the command range (value >= 32768, used in message maps
  where a collision fires the wrong handler, and today zero-duplicate) is
  enforced unique. Any value in that range defined by 2+ names fails.

  Check 2 -- _APS_NEXT_* monotonicity: Visual Studio allocates fresh IDs
  from these markers, so they must never move backwards below the known
  baselines, otherwise a future auto-allocated ID collides with an existing
  hand-added one.

  Read-only: never modifies resource.h. Runs on Windows PowerShell 5.1 and
  PowerShell 7.
#>
param(
  [string]$ResourceHeader = (Join-Path (Split-Path $PSScriptRoot -Parent) 'resource.h')
)

$ErrorActionPreference = 'Stop'

# Baselines taken from resource.h tail (_APS_NEXT_* block); values must only grow.
$nextFloors = @{
  RESOURCE = 394
  COMMAND  = 33002
  CONTROL  = 2174
}
$commandRangeMin = 32768

if (-not (Test-Path -LiteralPath $ResourceHeader)) {
  Write-Output ("::error file={0}::resource header not found: {1}" -f $ResourceHeader, $ResourceHeader)
  exit 1
}

$byValue = @{}
$nextValues = @{}
foreach ($line in [System.IO.File]::ReadAllLines($ResourceHeader)) {
  $m = [regex]::Match($line, '^\s*#define\s+([A-Za-z_]\w*)\s+(\d+)\s*(?://.*)?$')
  if (-not $m.Success) { continue }
  $name = $m.Groups[1].Value
  $value = [int]$m.Groups[2].Value

  $n = [regex]::Match($name, '^_APS_NEXT_(RESOURCE|COMMAND|CONTROL)_VALUE$')
  if ($n.Success) {
    $nextValues[$n.Groups[1].Value] = $value
    continue
  }
  if ($name -like '_APS_*') { continue }  # other studio markers, not real IDs
  if (-not $byValue.ContainsKey($value)) { $byValue[$value] = @() }
  $byValue[$value] += $name
}

$failed = $false

foreach ($value in ($byValue.Keys | Sort-Object)) {
  $names = @($byValue[$value] | Sort-Object -Unique)
  if ($names.Count -gt 1 -and $value -ge $commandRangeMin) {
    $failed = $true
    Write-Output ("::error file=resource.h::reused command ID {0}: {1}" -f $value, ($names -join ', '))
    Write-Output ('  REUSED ID {0}: {1}' -f $value, ($names -join ', '))
  }
}

foreach ($kind in @('RESOURCE', 'COMMAND', 'CONTROL')) {
  $floor = $nextFloors[$kind]
  if (-not $nextValues.ContainsKey($kind)) {
    $failed = $true
    Write-Output ("::error file=resource.h::missing _APS_NEXT_{0}_VALUE marker" -f $kind)
    continue
  }
  $actual = $nextValues[$kind]
  Write-Output ('_APS_NEXT_{0}_VALUE = {1} (floor {2})' -f $kind, $actual, $floor)
  if ($actual -lt $floor) {
    $failed = $true
    Write-Output ("::error file=resource.h::_APS_NEXT_{0}_VALUE regressed: {1} < baseline {2}" -f $kind, $actual, $floor)
  }
}

if ($failed) {
  Write-Output 'resource-id check FAILED.'
  exit 1
}

Write-Output 'resource-id check OK.'
exit 0
