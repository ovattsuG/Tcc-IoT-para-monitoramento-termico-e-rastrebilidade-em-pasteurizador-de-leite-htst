# Plano — Aplicação Edge-HTST (fase Plan do SDD)

Data: 10/11/2026 · Baseado em `spec/SRS_Aplicacao-HTST_v1.0.md`

A arquitetura, as tabelas e os requisitos já estão na SRS. Este plano registra **como** cada peça vai cumprir os SW-xx: as escolhas que a SRS deixa em aberto, com a alternativa descartada.

## Decisões

| # | Decisão | Alternativa descartada | Motivo | SW |
|---|---|---|---|---|
| 1 | Desenvolver e testar tudo no PC de bancada; depois repetir no notebook da fábrica seguindo `docs/instalacao-aplicacao.md` | Montar direto na fábrica | S-D4 em aberto; cada teste dependeria de ir à fábrica | — |
| 2 | Node-RED recebe a mensagem como **texto** e faz o `JSON.parse` dentro do nó de validação | Deixar o nó MQTT converter o JSON | Assim a mensagem malformada é capturada e vai para `rejeitadas` com o motivo, em vez de virar só um erro no log | SW-05 |
| 3 | Validação num nó `function`: campos obrigatórios, tipos, valores permitidos (`fdv`, `modo`), temperatura nula só com `*_valida = 0` | Gravar tudo e filtrar no Grafana | O banco guarda só o que é leitura de verdade; o resto fica em `rejeitadas` | SW-02, SW-05 |
| 4 | Gravação com `INSERT ... ON CONFLICT DO NOTHING`, **sem alvo** (a única chave única é `(device_id, seq)`) | Consultar antes se o `seq` já existe; `ON CONFLICT (device_id, seq)`, que exige SELECT nessas colunas (visto no teste do SW-04, 10/11/2026) | Uma só operação, sem corrida, e o `nodered` fica só com INSERT | SW-03, SW-04 |
| 5 | Nó de PostgreSQL da paleta: `node-red-contrib-postgresql` (versão anotada na instalação), com a consulta parametrizada | Montar o SQL concatenando texto | Parâmetros evitam injeção de SQL e problemas com aspas | SW-02 |
| 6 | Três usuários no banco: `htst_admin` (dono, cria as tabelas), `nodered` (só INSERT), `grafana` (só SELECT) | Um usuário só | É assim que o SW-04 se prova: um UPDATE de `nodered` ou `grafana` falha | SW-04 |
| 7 | Sessão MQTT persistente do Node-RED no Mosquitto (client id fixo, sem "clean session", QoS 1) e `persistence true` no `mosquitto.conf` | Sessão limpa (padrão) | Se o Node-RED reiniciar, o Mosquitto guarda as mensagens até ele voltar; com sessão limpa elas se perdem | SW-02 |
| 8 | Erro de banco (PostgreSQL fora do ar) vai para o log do Node-RED por um nó `catch` | Ignorar | Regra do projeto: nenhum erro em silêncio. A cópia completa continua no CSV do SD do nó | SW-05 |
| 9 | Mosquitto: `listener 1883` na rede local, firewall do Windows liberando a 1883 só para a rede local ("Privada") | Expor para fora | SW-01; sem TLS, a proteção é a rede local | SW-01 |
| 10 | Grafana com fonte de dados PostgreSQL (usuário `grafana`), fuso do painel `America/Sao_Paulo`, atualização a cada 5 s | Plugin MQTT direto no Grafana | O banco é a fonte única (SRS, seção 2); o dashboard ao vivo e o histórico leem o mesmo lugar | SW-06, SW-14 |
| 11 | "Nó parado": painel com o tempo desde o último `recebido_em`, vermelho acima de 60 s | Alerta do Grafana | Mais simples e é o que o critério pede (painel vermelho) | SW-08 |
| 12 | Ocorrências (início e fim de cada condição) calculadas por consulta SQL sobre `leituras` | Tabela própria de ocorrências | A SRS manda uma fonte única de dados | SW-10 |
| 13 | Node-RED ligado com o PC pelo **Agendador de Tarefas** do Windows ("ao iniciar o sistema", reiniciar se falhar) | NSSM (programa extra) | Já vem no Windows; Mosquitto, PostgreSQL e Grafana já se instalam como serviço | — |
| 14 | Backup diário com `pg_dump` pelo Agendador de Tarefas, alguns minutos depois de o notebook ligar, numa pasta do Google Drive para computador (conta do dono) | Pendrive fixo; horário fixo | O notebook desliga em horário variável; o Drive tira a cópia da fábrica sem custo (decisão de 10/11/2026) | SW-13 |
| 16 | Registro sem hora (`timestamp_utc` nulo) é gravado em `leituras` com a hora nula | Mandar para `rejeitadas` | Formato válido pela seção 7 do FRS; raro (RTC e NTP falhando juntos) | SW-02 |
| 15 | `credentialSecret` próprio no `settings.js` do Node-RED (fora do Git) | Chave gerada pelo sistema | Com a chave gerada, a senha do banco não abre no notebook da fábrica | — |

## O que vai para o repositório (pasta `aplicacao/`)

```
aplicacao/
├── sql/
│   ├── 01_tabelas.sql       leituras, rejeitadas, chave única
│   └── 02_usuarios.sql      nodered e grafana com as permissões (sem senha)
├── nodered/flows.json       exportado do Node-RED
├── grafana/*.json           dashboards exportados
├── mosquitto/mosquitto.conf
└── testes/                  mensagens de teste com mosquitto_pub (repetida, malformada, campo faltando)
```

**Fora do Git:** `flows_cred.json`, `settings.js` (tem o `credentialSecret`), senhas dos usuários do banco e do Grafana.

## Ordem de implementação

1. PostgreSQL: instalar, criar o banco, as tabelas e os usuários (SW-04).
2. Node-RED: fluxo MQTT → validação → INSERT, com `rejeitadas` e `catch` (SW-02, SW-03, SW-05).
3. Mosquitto: `listener`, `persistence`, firewall (SW-01).
4. Grafana: fonte de dados, dashboard ao vivo, alarmes, nó parado (SW-06, SW-07, SW-08, SW-14).
5. Grafana: histórico, relatório, login, anotação "alarme visto" (SW-09 a SW-12).
6. Serviços e backup (SW-13).
7. Teste de 24 h junto com o firmware (FW-09 a FW-11 e SW-02).

## Perguntas respondidas (10/11/2026)

- Registro sem hora: grava em `leituras` (decisão 16).
- Backup: ao ligar o notebook, para o Google Drive (decisão 14; resolve S-D3).
