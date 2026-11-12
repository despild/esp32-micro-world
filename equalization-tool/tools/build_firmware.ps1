param(
  [string]$ArduinoCli = "$env:LOCALAPPDATA/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe",
  [string]$ArduinoData = "$env:LOCALAPPDATA/Arduino15",
  [string]$Python = 'python'
)
$ErrorActionPreference = 'Stop'
$eqRoot = Split-Path -Parent $PSScriptRoot
& $Python (Join-Path $PSScriptRoot 'embed_web.py')
if ($LASTEXITCODE -ne 0) { throw 'Web embedding failed' }
$eqConfig = Join-Path $eqRoot 'local-arduino.yaml'
$eqBuild = Join-Path $eqRoot 'build/EqualizationTool'
$eqUser = (Join-Path $eqRoot 'build/arduino-user').Replace('\','/')
$eqData = $ArduinoData.Replace('\','/')
"directories:`n  data: $eqData`n  user: $eqUser" | Set-Content -LiteralPath $eqConfig -Encoding utf8
& $ArduinoCli --config-file $eqConfig compile --fqbn 'esp32:esp32:esp32c3:CDCOnBoot=cdc,FlashSize=4M' --build-path $eqBuild (Join-Path $eqRoot 'firmware/EqualizationTool')
if ($LASTEXITCODE -ne 0) { throw 'Equalization firmware build failed' }
$eqImage = Get-Item -LiteralPath (Join-Path $eqBuild 'EqualizationTool.ino.bin')
if ($eqImage.Length -gt 1048576) { throw 'Application exceeds existing 1 MiB partition; do not flash' }
Write-Output "Application: $($eqImage.Length) / 1048576 bytes"
