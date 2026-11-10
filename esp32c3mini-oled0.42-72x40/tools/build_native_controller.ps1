param([string]$OutputDirectory='')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$dest=if($OutputDirectory){[IO.Path]::GetFullPath($OutputDirectory)}else{Join-Path $projectRoot 'bin'}
New-Item -ItemType Directory -Path $dest -Force | Out-Null
$source=Join-Path $projectRoot 'controller\native\PocketWorld.cs'
$manifest=Join-Path $projectRoot 'controller\native\app.manifest'
$exe=Join-Path $dest 'PocketWorld.exe'
& $compiler /nologo /target:winexe /optimize+ /platform:anycpu "/out:$exe" "/win32manifest:$manifest" /reference:System.dll /reference:System.Core.dll /reference:System.Drawing.dll /reference:System.Windows.Forms.dll /reference:System.Management.dll /reference:System.Web.Extensions.dll $source
if($LASTEXITCODE -ne 0){throw 'Native controller build failed'}
Get-FileHash -Algorithm SHA256 -LiteralPath "$dest/PocketWorld.exe"
