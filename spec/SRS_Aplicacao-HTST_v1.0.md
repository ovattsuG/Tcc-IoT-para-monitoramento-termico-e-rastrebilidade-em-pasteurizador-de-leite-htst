# Especificação de Requisitos de Software — Aplicação Edge-HTST

| Campo | Valor |
|---|---|
| Projeto | Arquitetura IoT para Pasteurizadores HTST |
| Software | Aplicação de recepção, armazenamento e visualização |
| Identificador | SW-HTST-01 |
| Versão | 1.0 |
| Data | 02/11/2026 |
| Autor | Gustavo Correa A. Moreira |
| Status | Em revisão |
| Depende de | FW-HTST-01 v1.5 (formato da telemetria, seção 7) |

---

## 1. Objetivo e escopo

A aplicação recebe a telemetria do nó Edge-HTST, guarda os dados sem alteração e os mostra em dashboard. É ela que substitui a retirada por pendrive e a montagem manual de planilhas e gráficos no Excel.

**Dentro do escopo**
- Broker MQTT, Node-RED, PostgreSQL e Grafana OSS, todos no computador da fábrica.
- Dashboard em tempo real, histórico por período, alarme visual, aviso de nó parado e relatório de auditoria.
- Login, registro de "alarme visto" e backup diário.

**Fora do escopo**
- **Notificação remota** (Telegram, e-mail ou similar). O alerta é físico, pela torre do nó, e visual, no dashboard.
- Relatório em PDF nativo do Grafana, que só existe nas versões Enterprise e Cloud. O PDF sai pela impressão do navegador.
- Backend e frontend próprios (possível evolução futura).
- Acesso pela internet e TLS: a aplicação só é acessível na rede local do laticínio.
- Comando de qualquer equipamento do processo.

## 2. Arquitetura

```
ESP32 (nó) ──Wi-Fi do laticínio──► Broker MQTT ──► Node-RED ──► PostgreSQL ◄── Grafana ◄── navegador (rede local)
   │                              └──────── computador da fábrica ────────┘
   └─ torre (alarme físico) e cartão SD (cópia local e reenvio)
```

- **Broker MQTT (Mosquitto):** recebe as mensagens do nó. Roda como serviço separado do Node-RED, para continuar no ar quando o Node-RED reinicia.
- **Node-RED:** assina o tópico, valida cada mensagem e grava no banco. **Não tem nenhuma regra de alarme:** as regras ficam no nó.
- **PostgreSQL:** guarda as leituras. É a fonte única dos dados mostrados.
- **Grafana:** só lê o banco. Faz os gráficos, o alarme visual, o relatório e o login.

## 3. Modelo de dados

**Tabela `leituras`:** uma linha por mensagem do nó.

| Coluna | Tipo | Observação |
|---|---|---|
| `device_id` | texto | |
| `seq` | inteiro longo | **Único junto com `device_id`** |
| `timestamp_utc` | timestamp com fuso | Hora do nó |
| `t1_c`, `t2_c` | decimal, **pode ser nulo** | Nulo quando a leitura é inválida |
| `t1_valida`, `t2_valida` | booleano | |
| `fdv` | texto | `DIRETO` ou `DESVIO` |
| `pressao_ok` | booleano | |
| `modo` | texto | `PRODUCAO` ou `LIMPEZA` |
| `alarmes` | texto | Lista separada por `\|`; vazio = OK |
| `recebido_em` | timestamp com fuso | Hora de gravação no banco |

**Tabela `rejeitadas`:** mensagens que não passaram na validação, com o conteúdo bruto, o motivo e a hora.

As ocorrências de alarme (início e fim de cada condição) **são calculadas a partir de `leituras` por consulta**, sem tabela própria, para manter uma única fonte dos dados.

## 4. Requisitos

| ID | Requisito | Critério de aceitação |
|---|---|---|
| SW-01 | Broker MQTT no computador da fábrica, acessível apenas na rede local | O nó publica, e um dispositivo fora da rede local não consegue conectar |
| SW-02 | O Node-RED assina `htst/+/leituras` e grava cada mensagem válida em `leituras` | Depois de 1 h, o número de linhas é igual ao número de `seq` publicados |
| SW-03 | Descartar mensagens repetidas pela chave única (`device_id`, `seq`) | Reenviar 100 mensagens já gravadas não gera nenhuma linha nova |
| SW-04 | Preservar o original: a aplicação só insere e lê, nunca altera nem apaga leituras | O usuário de banco do Node-RED só tem permissão de INSERT; o do Grafana, só de SELECT. Um UPDATE feito por qualquer um dos dois falha |
| SW-05 | Mensagem inválida (JSON malformado ou campo faltando) vai para `rejeitadas` com o motivo, e nunca é descartada em silêncio | Uma mensagem malformada aparece em `rejeitadas` |
| SW-06 | Dashboard em tempo real com T1 e T2 no mesmo gráfico, linha de 72 °C, estado da FDV, da pressão e dos alarmes, atualizado a cada 5 s no máximo | Inspeção do painel em operação |
| SW-07 | Alarme visual: as condições `SUBTEMP_SEM_DESVIO`, `PRESSAO` e `FALHA_T1` aparecem em vermelho; `FALHA_T2`, `WDT` e `FALHA_SD`, em destaque de aviso | Do evento no nó até a exibição no dashboard em até **10 s** (TCC, seção 3.3) |
| SW-08 | Nó parado: o painel "última leitura" fica vermelho quando passam mais de 60 s sem dados | Desligar o nó: o painel fica vermelho em até 70 s |
| SW-09 | Histórico de qualquer período, com as leituras inválidas como **lacuna** na linha e os alarmes como faixas sobre o gráfico | Os dados dos cenários 5 e 6 da bancada aparecem como lacuna, e não como valor |
| SW-10 | Dashboard "Relatório" com seletor de período: gráfico de T1 e T2; mínimo, máximo e média; tempo abaixo de 72 °C com desvio e sem desvio; lista de ocorrências com início e fim. Exportação em CSV pelo painel e em PDF pela impressão do navegador | Gerar o relatório de um dia de operação nos dois formatos |
| SW-11 | Acesso pelo login do Grafana: contas individuais, todas com o mesmo papel de uso, mais uma conta de administrador [S-D2] | Sem login, o dashboard não abre |
| SW-12 | "Alarme visto": o usuário registra uma anotação no gráfico, e o Grafana guarda automaticamente o autor e a hora | A anotação aparece com o nome do usuário |
| SW-13 | Backup automático diário do banco para fora do computador da aplicação [S-D3] | Restaurar um backup em outra máquina e conferir as leituras |
| SW-14 | Gravar em UTC e exibir no horário de Brasília (America/Sao_Paulo) | Um evento de 13:00 UTC aparece às 10:00 |

**Por que contas individuais com um papel só:** se todos usarem a mesma conta, a anotação "alarme visto" registra só o nome da conta, e não quem viu. Contas individuais com o mesmo papel não aumentam a complexidade e mantêm o registro atribuível (TCC, seção 2.4).

## 5. Decisões em aberto

| ID | Decisão | Como resolver |
|---|---|---|
| S-D2 | Se o papel Viewer do Grafana pode criar anotações. Se não puder, usar o papel Editor restrito à pasta do dashboard | Testar na instalação |
| S-D3 | Destino do backup (outro disco, pendrive fixo ou outro computador) | Combinar com a fábrica |
| S-D4 | Sistema operacional do computador da fábrica | Verificar na fábrica |
| S-D5 | Se o computador fica ligado 24 h. Se desligar, o nó guarda no cartão SD e reenvia (FW-11, até 24 h) | Confirmar com a fábrica |

## 6. Limitações (registrar no TCC)

- **Sem notificação remota:** o alarme só chega a quem está perto da torre ou com o dashboard aberto.
- O relatório em PDF depende da impressão do navegador.
- Não há TLS: a segurança depende de a rede local do laticínio ser restrita.
