# Instalação da aplicação (Mosquitto, Node-RED, PostgreSQL, Grafana)

Registro do que foi instalado e configurado, para repetir no computador da fábrica. Especificação: `spec/SRS_Aplicacao-HTST_v1.0.md`. O passo a passo genérico de cada programa está no link oficial; aqui fica só o que fizemos e o que difere do padrão.

## Node-RED

Passo a passo oficial: https://nodered.org/docs/getting-started/windows

**10/11/2026, PC de bancada (Windows 11):**

| Item | Valor |
|---|---|
| Node.js | v24.19.0 (LTS, instalador `.msi`, sem "Tools for Native Modules") |
| npm | 10.9.9 |
| Node-RED | v5.0.9 |
| Instalação | `npm install -g node-red` |
| Iniciar | `node-red` no PowerShell (por enquanto só com o terminal aberto) |
| Editor | http://127.0.0.1:1880 |
| Pasta do usuário | `C:\Users\<usuário>\.node-red` (`settings.js`, `flows.json`) |
| Nó de banco | `node-red-contrib-postgresql` 0.16.2 (menu ☰ → Manage palette → Install) |
| Chave das credenciais | `credentialSecret` próprio no `settings.js` (fora do Git; guardar junto com as senhas) |

Conferido: o Node-RED subiu com `Server now running at http://127.0.0.1:1880/` e `Started flows`.

**Pendente:** definir `credentialSecret` no `settings.js`; rodar como serviço (ligar com o PC).

## PostgreSQL

Download oficial: https://www.postgresql.org/download/windows/ (instalador da EDB)

**10/11/2026, PC de bancada:**

| Item | Valor |
|---|---|
| Versão | PostgreSQL 18 |
| Componentes | PostgreSQL Server, pgAdmin 4, Command Line Tools (sem Stack Builder) |
| Porta | 5432 |
| Serviço | `postgresql-x64-18`, Automático (conferido em execução depois de reiniciar o PC) |
| Superusuário | `postgres` (senha fora do repositório) |
| Pasta | `C:\Program Files\PostgreSQL\18` |

Conferido: o pgAdmin 4 conecta no servidor "PostgreSQL 18".

## Mosquitto

Download oficial: https://mosquitto.org/download/ (instalado no bloco 4 do firmware; o instalador cria o serviço `mosquitto`, Automático).

**10/11/2026, PC de bancada:**

| Item | Valor |
|---|---|
| Versão | 2.1.2 |
| Configuração | `aplicacao/mosquitto/mosquitto.conf` copiada para `C:\Program Files\mosquitto\mosquitto.conf` |
| Persistência | `C:\mqtt\mosquitto.db` (grava ao parar o serviço e a cada 30 min) |
| Log | `C:\mqtt\mosquitto.log` |
| Firewall | regra "Mosquitto 1883" (entrada, TCP 1883) só no perfil **Privado**; as regras "mosquitto" do perfil Público, criadas pelo Windows na instalação, desativadas |
| Rede | Wi-Fi como "Privada" (Configurações → Rede e Internet → Wi-Fi → propriedades) |

Comandos (PowerShell como administrador):
```powershell
Copy-Item "<repositório>\aplicacao\mosquitto\mosquitto.conf" "C:\Program Files\mosquitto\mosquitto.conf"
Restart-Service mosquitto
Get-NetFirewallRule -DisplayName "mosquitto" | Disable-NetFirewallRule
```

Conferido no log depois do reinício: `htst-01` (ESP) conectado com sessão limpa (`c1`) e `nodered-htst` com sessão persistente (`c0`).

## Grafana OSS

Download oficial: https://grafana.com/grafana/download?edition=oss (Windows, instalador `.msi`).

**11/11/2026, PC de bancada:**

| Item | Valor |
|---|---|
| Versão | 13.3.1 (OSS) |
| Serviço | `Grafana`, Automático |
| Endereço | http://localhost:3000 (escuta em todas as placas de rede) |
| Administrador | `admin`, senha trocada no primeiro acesso (fora do repositório) |

Conferido: `GET /api/health` → `database: ok`, versão 13.3.1.
