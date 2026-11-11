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
