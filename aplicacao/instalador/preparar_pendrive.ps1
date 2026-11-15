# Monta a pasta do pendrive de instalação. Rodar no PC de desenvolvimento, com internet:
#   powershell -ExecutionPolicy Bypass -File aplicacao\instalador\preparar_pendrive.ps1 -Destino E:\htst-instalacao
# Depois, no PC da fábrica: botão direito em instalar.cmd → Executar como administrador.
param(
  [Parameter(Mandatory)] [string]$Destino,
  [string]$Instaladores = "$env:USERPROFILE\Downloads"
)
$ErrorActionPreference = 'Stop'
$app = Resolve-Path "$PSScriptRoot\.."

New-Item -ItemType Directory -Force "$Destino\instaladores", "$Destino\config", "$Destino\grafana", "$Destino\nodered" | Out-Null

foreach ($padrao in 'postgresql-*-windows-x64.exe', 'node-v*-x64.msi', 'mosquitto-*-install-windows-x64.exe', 'grafana_*_windows_amd64.msi') {
  $f = Get-ChildItem $Instaladores -Filter $padrao | Sort-Object LastWriteTime | Select-Object -Last 1
  if (-not $f) { throw "Não achei $padrao em $Instaladores" }
  Copy-Item $f.FullName "$Destino\instaladores\" -Force
  Write-Host "instalador: $($f.Name)"
}

# Node-RED e o nó do banco já baixados aqui: no PC da fábrica não precisa de internet.
# npm instala no disco do PC e só depois copia: direto no pendrive (exFAT/FAT32) a extração falha (TAR_ENTRY_ERROR).
$temp = Join-Path $env:TEMP 'htst-nodered'
New-Item -ItemType Directory -Force $temp | Out-Null
Push-Location $temp
try {
  if (-not (Test-Path package.json)) { Set-Content package.json '{ "name": "htst-nodered", "private": true }' -Encoding ASCII }
  npm install --no-audit --no-fund node-red@5.0.9 node-red-contrib-postgresql@0.16.2
  if ($LASTEXITCODE -ne 0) { throw 'npm install falhou' }
} finally { Pop-Location }
Remove-Item "$Destino\nodered" -Recurse -Force -ErrorAction SilentlyContinue   # sobra da tentativa anterior
robocopy $temp "$Destino\nodered" /E /NFL /NDL /NJH /NJS /NP | Out-Null
if ($LASTEXITCODE -ge 8) { throw "cópia do Node-RED para o pendrive falhou (robocopy $LASTEXITCODE)" }

Copy-Item "$app\sql\01_tabelas.sql", "$app\sql\02_usuarios.sql", "$app\mosquitto\mosquitto.conf" "$Destino\config\" -Force
node "$PSScriptRoot\preparar_arquivos.js" $app $Destino
if ($LASTEXITCODE -ne 0) { throw 'preparar_arquivos.js falhou' }
Copy-Item "$PSScriptRoot\instalar.ps1", "$PSScriptRoot\instalar.cmd" $Destino -Force
Write-Host "`nPendrive pronto em $Destino" -ForegroundColor Green
