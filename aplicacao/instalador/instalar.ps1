# Instala e configura a aplicação HTST num PC Windows novo: PostgreSQL, Mosquitto, Grafana, Node.js e Node-RED,
# todos como serviços que sobem com o Windows, sem login (SW-01 a SW-12). Rodar pelo instalar.cmd, como administrador.
# Pode rodar de novo se algo falhar: cada passo pula o que já está feito.
#Requires -RunAsAdministrator
param([int]$PortaGrafana = 3000)

$ErrorActionPreference = 'Stop'
$raiz = $PSScriptRoot
$pastaNr = 'C:\htst\nodered'
$OutputEncoding = [Text.UTF8Encoding]::new($false)   # texto mandado ao psql pelo pipe (padrão do 5.1 é ASCII)

function Passo($texto) { Write-Host "`n=== $texto" -ForegroundColor Cyan }

function Instalador($padrao) {
  $f = Get-ChildItem (Join-Path $raiz 'instaladores') -Filter $padrao | Select-Object -First 1
  if (-not $f) { throw "Falta o instalador $padrao na pasta instaladores" }
  $f.FullName
}

function Rodar($exe, $argumentos) {
  $p = Start-Process $exe -ArgumentList $argumentos -Wait -PassThru
  if ($p.ExitCode -notin 0, 3010) { throw "$(Split-Path $exe -Leaf) terminou com código $($p.ExitCode)" }   # 3010 = ok, pede reiniciar
}

function PedirSenha($para) {
  while ($true) {
    $a = [Net.NetworkCredential]::new('', (Read-Host "Senha para $para" -AsSecureString)).Password
    $b = [Net.NetworkCredential]::new('', (Read-Host 'Repita' -AsSecureString)).Password
    if ($a -ne $b) { Write-Host 'As senhas não conferem.' -ForegroundColor Yellow; continue }
    # Aspas quebrariam a linha de comando do instalador do PostgreSQL.
    if ($a.Length -lt 8 -or $a -match '["'']') { Write-Host 'Use 8 ou mais caracteres, sem aspas.' -ForegroundColor Yellow; continue }
    return $a
  }
}

function PortaAberta($porta) {
  try { $c = [Net.Sockets.TcpClient]::new('127.0.0.1', $porta); $c.Close(); $true } catch { $false }
}

try {
  Passo 'Senhas (nenhuma fica gravada no pendrive)'
  $senhaPostgres  = PedirSenha 'o superusuário postgres do banco'
  $senhaNodered   = PedirSenha 'o usuário nodered do banco (só grava)'
  $senhaGrafanaDb = PedirSenha 'o usuário grafana do banco (só lê)'
  $senhaAdmin     = PedirSenha 'o admin do Grafana'
  $loginOperador  = Read-Host 'Login do operador no Grafana (ex.: operador)'
  $senhaOperador  = PedirSenha "o operador $loginOperador no Grafana"

  # ---------------------------------------------------------------- PostgreSQL (SW-02 a SW-05)
  Passo 'PostgreSQL'
  if (-not (Get-Service 'postgresql-x64-*' -ErrorAction SilentlyContinue)) {
    Rodar (Instalador 'postgresql-*-windows-x64.exe') "--mode unattended --unattendedmodeui minimal --superpassword `"$senhaPostgres`" --serverport 5432 --disable-components stackbuilder"
  }
  $servicoPg = (Get-Service 'postgresql-x64-*' | Select-Object -First 1).Name
  Set-Service $servicoPg -StartupType Automatic
  $psql = Get-ChildItem 'C:\Program Files\PostgreSQL\*\bin\psql.exe' | Select-Object -Last 1 -ExpandProperty FullName
  $env:PGPASSWORD = $senhaPostgres
  $env:PGCLIENTENCODING = 'UTF8'

  function Sql([string]$banco, [string]$comando, [string[]]$extra = @()) {
    $saida = $comando | & $psql -h 127.0.0.1 -U postgres -d $banco -v ON_ERROR_STOP=1 -q -t -A @extra
    if ($LASTEXITCODE -ne 0) { throw "psql falhou no banco $banco" }
    $saida
  }
  if (-not (Sql 'postgres' "SELECT 1 FROM pg_database WHERE datname = 'htst';")) { Sql 'postgres' 'CREATE DATABASE htst;' | Out-Null }
  if (-not (Sql 'htst' "SELECT to_regclass('public.leituras');")) { Sql 'htst' (Get-Content "$raiz\config\01_tabelas.sql" -Raw -Encoding UTF8) | Out-Null }
  if (-not (Sql 'htst' "SELECT 1 FROM pg_roles WHERE rolname = 'nodered';")) { Sql 'htst' (Get-Content "$raiz\config\02_usuarios.sql" -Raw -Encoding UTF8) | Out-Null }
  # :'s1' = variável do psql já entre aspas e escapada: a senha nunca é colada no texto do SQL.
  Sql 'htst' "ALTER ROLE nodered PASSWORD :'s1'; ALTER ROLE grafana PASSWORD :'s2';" @('-v', "s1=$senhaNodered", '-v', "s2=$senhaGrafanaDb") | Out-Null
  Write-Host 'banco htst, tabelas e usuários ok'

  # ---------------------------------------------------------------- Mosquitto (SW-01)
  Passo 'Mosquitto'
  $pastaMosq = 'C:\Program Files\mosquitto'
  if (-not (Test-Path "$pastaMosq\mosquitto.exe")) { Rodar (Instalador 'mosquitto-*-install-windows-x64.exe') '/S' }
  if (-not (Get-Service mosquitto -ErrorAction SilentlyContinue)) { & "$pastaMosq\mosquitto.exe" install }
  New-Item -ItemType Directory -Force 'C:\mqtt' | Out-Null
  Copy-Item "$raiz\config\mosquitto.conf" "$pastaMosq\mosquitto.conf" -Force
  Set-Service mosquitto -StartupType Automatic
  Restart-Service mosquitto
  if (-not (Get-NetFirewallRule -DisplayName 'Mosquitto 1883' -ErrorAction SilentlyContinue)) {
    New-NetFirewallRule -DisplayName 'Mosquitto 1883' -Direction Inbound -Protocol TCP -LocalPort 1883 -Profile Private -Action Allow | Out-Null
  }
  Get-NetFirewallRule -DisplayName 'mosquitto' -ErrorAction SilentlyContinue | Disable-NetFirewallRule
  Write-Host 'mosquitto ok (porta 1883 liberada só na rede Privada)'

  # ---------------------------------------------------------------- Grafana (SW-06 a SW-11)
  Passo 'Grafana'
  if (-not (Get-Service Grafana -ErrorAction SilentlyContinue)) { Rodar 'msiexec.exe' "/i `"$(Instalador 'grafana_*_windows_amd64.msi')`" /qn" }
  Set-Content 'C:\Program Files\GrafanaLabs\grafana\conf\custom.ini' -Encoding ASCII -Value @(
    '[server]', "http_port = $PortaGrafana", '', '[auth.anonymous]', 'enabled = false', '', '[users]', 'allow_sign_up = false')
  Set-Service Grafana -StartupType Automatic
  Restart-Service Grafana

  $url = "http://127.0.0.1:$PortaGrafana"
  $prazo = (Get-Date).AddMinutes(2)
  while (-not (PortaAberta $PortaGrafana)) {
    if ((Get-Date) -gt $prazo) { throw "O Grafana não abriu a porta $PortaGrafana em 2 min" }
    Start-Sleep -Seconds 3
  }
  Start-Sleep -Seconds 5

  function Api($metodo, $caminho, $corpo, $senha = $senhaAdmin) {
    $auth = 'Basic ' + [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes("admin:$senha"))
    $p = @{ Method = $metodo; Uri = "$url$caminho"; Headers = @{ Authorization = $auth }; ContentType = 'application/json; charset=utf-8' }
    if ($corpo) {
      $texto = if ($corpo -is [string]) { $corpo } else { $corpo | ConvertTo-Json -Depth 10 }
      $p.Body = [Text.Encoding]::UTF8.GetBytes($texto)
    }
    Invoke-RestMethod @p
  }
  function Existe($caminho) { try { Api GET $caminho | Out-Null; $true } catch { $false } }

  # Grafana novo vem com admin/admin: troca já na instalação.
  $senhaPadrao = $true
  try { Api GET '/api/user' $null 'admin' | Out-Null } catch { $senhaPadrao = $false }
  if ($senhaPadrao) { Api PUT '/api/user/password' @{ oldPassword = 'admin'; newPassword = $senhaAdmin } 'admin' | Out-Null }
  if (-not (Existe '/api/user')) { throw 'A senha do admin do Grafana não confere' }

  # Mesmo uid do PC de desenvolvimento: os dashboards apontam para ele. A senha fica criptografada no Grafana.
  $fonte = @{ name = 'htst'; uid = 'fg3xub4ujtiwwf'; type = 'grafana-postgresql-datasource'; access = 'proxy'
    url = '127.0.0.1:5432'; user = 'grafana'
    jsonData = @{ database = 'htst'; sslmode = 'disable'; timeInterval = '1s' }
    secureJsonData = @{ password = $senhaGrafanaDb } }
  if (Existe '/api/datasources/uid/fg3xub4ujtiwwf') { Api PUT '/api/datasources/uid/fg3xub4ujtiwwf' $fonte | Out-Null }
  else { Api POST '/api/datasources' $fonte | Out-Null }

  # Dashboard que já existe não é sobrescrito: apagar levaria junto as anotações "alarme visto" (SW-12).
  $base = '/apis/dashboard.grafana.app/v2/namespaces/default/dashboards'
  foreach ($arq in Get-ChildItem "$raiz\grafana\*.json") {
    $texto = Get-Content $arq.FullName -Raw -Encoding UTF8
    $nome = ($texto | ConvertFrom-Json).metadata.name
    if (-not (Existe "$base/$nome")) { Api POST $base $texto | Out-Null; Write-Host "dashboard $($arq.Name) importado" }
    # A API v2 não dá a permissão padrão (Editor edita, Viewer vê): sem isto só o admin enxerga o dashboard.
    Api POST "/api/dashboards/uid/$nome/permissions" '{"items":[{"role":"Viewer","permission":1},{"role":"Editor","permission":2}]}' | Out-Null
  }

  if (-not (Existe "/api/users/lookup?loginOrEmail=$loginOperador")) {
    $u = Api POST '/api/admin/users' @{ name = $loginOperador; login = $loginOperador; password = $senhaOperador; OrgId = 1 }
    Api PATCH "/api/org/users/$($u.id)" @{ role = 'Editor' } | Out-Null   # Editor: mínimo para criar anotação (SW-12)
  }
  Write-Host "grafana ok em http://localhost:$PortaGrafana"

  # ---------------------------------------------------------------- Node.js e Node-RED (SW-02, SW-03, SW-05)
  Passo 'Node.js e Node-RED'
  $node = 'C:\Program Files\nodejs\node.exe'
  if (-not (Test-Path $node)) { Rodar 'msiexec.exe' "/i `"$(Instalador 'node-v*-x64.msi')`" /qn" }
  if (-not (Test-Path "$pastaNr\node_modules\node-red\red.js")) {
    robocopy "$raiz\nodered" $pastaNr /E /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { throw 'Falha ao copiar o Node-RED para C:\htst' }   # robocopy: 0 a 7 = sucesso
  }
  # O fluxo só é copiado na primeira vez: um ajuste feito na fábrica não é perdido ao rodar de novo.
  if (-not (Test-Path "$pastaNr\flows.json")) { Copy-Item "$raiz\config\flows.json" "$pastaNr\flows.json" }
  New-Item -ItemType Directory -Force "$pastaNr\logs" | Out-Null

  # shortcut: usa o nssm que o instalador do Grafana traz; se uma versão futura não trouxer, pôr o nssm.exe em instaladores\.
  $nssm = Get-ChildItem 'C:\Program Files\GrafanaLabs\svc-*\nssm.exe', "$raiz\instaladores\nssm.exe" -ErrorAction SilentlyContinue |
    Select-Object -First 1 -ExpandProperty FullName
  if (-not $nssm) { throw 'nssm.exe não encontrado' }
  if (-not (Get-Service NodeRED -ErrorAction SilentlyContinue)) {
    # --userDir: o serviço roda como SYSTEM; sem isso o Node-RED procura o fluxo na pasta do SYSTEM e sobe vazio.
    # uiHost=127.0.0.1: o editor só abre neste PC, ninguém na rede da fábrica altera o fluxo.
    & $nssm install NodeRED $node "$pastaNr\node_modules\node-red\red.js" --userDir $pastaNr --port 1880 -D uiHost=127.0.0.1
  }
  & $nssm set NodeRED AppDirectory $pastaNr | Out-Null
  # shortcut: a senha fica no registro do serviço (lido só por administradores e SYSTEM), como o flows_cred.json + settings.js ficariam no disco.
  & $nssm set NodeRED AppEnvironmentExtra 'HTST_DB_USUARIO=nodered' "HTST_DB_SENHA=$senhaNodered" | Out-Null
  & $nssm set NodeRED DependOnService mosquitto $servicoPg | Out-Null
  & $nssm set NodeRED Start SERVICE_AUTO_START | Out-Null
  & $nssm set NodeRED AppStdout "$pastaNr\logs\nodered.log" | Out-Null
  & $nssm set NodeRED AppStderr "$pastaNr\logs\nodered.log" | Out-Null
  & $nssm set NodeRED AppRotateFiles 1 | Out-Null
  & $nssm set NodeRED AppRotateOnline 1 | Out-Null
  & $nssm set NodeRED AppRotateBytes 10485760 | Out-Null   # 10 MB por arquivo de log
  Restart-Service NodeRED
  $prazo = (Get-Date).AddMinutes(1)
  while (-not (PortaAberta 1880) -and (Get-Date) -lt $prazo) { Start-Sleep -Seconds 2 }

  # ---------------------------------------------------------------- Conferência
  Passo 'Conferência'
  $tudoOk = $true
  function Conferir($nome, $ok) {
    if ($ok) { Write-Host "  OK      $nome" -ForegroundColor Green } else { Write-Host "  FALHOU  $nome" -ForegroundColor Red; $script:tudoOk = $false }
  }
  foreach ($s in 'mosquitto', $servicoPg, 'Grafana', 'NodeRED') {
    $svc = Get-Service $s
    Conferir "serviço $s (rodando, automático)" ($svc.Status -eq 'Running' -and $svc.StartType -eq 'Automatic')
  }
  foreach ($porta in 1883, 5432, 1880, $PortaGrafana) { Conferir "porta $porta" (PortaAberta $porta) }
  $saude = try { (Api GET '/api/datasources/uid/fg3xub4ujtiwwf/health').status } catch { 'ERRO' }
  Conferir 'Grafana conecta no banco como grafana' ($saude -eq 'OK')
  $log = Get-Content "$pastaNr\logs\nodered.log" -Tail 200 -ErrorAction SilentlyContinue
  Conferir 'Node-RED iniciou o fluxo (Started flows)' ($log -match 'Started flows')

  Write-Host "`nEndereços deste PC (o ESP precisa do IP fixo no segredos.h):"
  Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.IPAddress -notlike '127.*' -and $_.IPAddress -notlike '169.254.*' } |
    ForEach-Object { Write-Host "  $($_.InterfaceAlias): $($_.IPAddress)" }
  Get-NetConnectionProfile | Where-Object NetworkCategory -ne 'Private' | ForEach-Object {
    Write-Host "  ATENÇÃO: a rede '$($_.Name)' está como $($_.NetworkCategory); o ESP só conecta com a rede como Privada." -ForegroundColor Yellow
  }
  Write-Host "`nFalta à mão: fixar o IP deste PC no roteador ou no Windows e gravar no ESP." -ForegroundColor Yellow
  if ($tudoOk) { Write-Host "`nINSTALAÇÃO OK" -ForegroundColor Green } else { Write-Host "`nHÁ ITENS COM FALHA (acima). Corrija e rode de novo." -ForegroundColor Red }
}
finally {
  Remove-Item Env:PGPASSWORD -ErrorAction SilentlyContinue
}
