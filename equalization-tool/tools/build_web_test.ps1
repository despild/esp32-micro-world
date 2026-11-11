param(
  [string]$ArduinoCli = "$env:LOCALAPPDATA/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe",
  [string]$ArduinoData = "$env:LOCALAPPDATA/Arduino15"
)
$ErrorActionPreference = 'Stop'
$eqRoot = Split-Path -Parent $PSScriptRoot
$eqConfig = Join-Path $eqRoot 'local-arduino.yaml'
$eqBuild = Join-Path $eqRoot 'build/WebQrTest'
$eqUser = (Join-Path $eqRoot 'build/arduino-user').Replace('\','/')
$eqData = $ArduinoData.Replace('\','/')
"directories:`n  data: $eqData`n  user: $eqUser" | Set-Content -LiteralPath $eqConfig -Encoding utf8
& $ArduinoCli --config-file $eqConfig compile --fqbn 'esp32:esp32:esp32c3:CDCOnBoot=cdc,FlashSize=4M' --build-path $eqBuild (Join-Path $eqRoot 'firmware/WebQrTest')
if ($LASTEXITCODE -ne 0) { throw 'Web test firmware build failed' }
$eqImage = Get-Item -LiteralPath (Join-Path $eqBuild 'WebQrTest.ino.bin')
if ($eqImage.Length -gt 1048576) { throw 'Application exceeds existing 1 MiB partition' }
