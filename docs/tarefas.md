# Tarefas

Status: Pendente · Em andamento · Implementado – aguardando bancada · Concluída

## Bloco 5 (FW-13, FW-14: watchdog e autoteste)

Data: 07/11/2026 · Base: `spec/FRS_Edge-HTST_v1.5.md`, seções 5 e 6

| # | Tarefa | Req. | Depende de | Critério de conclusão | Status |
|---|---|---|---|---|---|
| 1 | Condição `WDT` no `lib/alarme` (bit 0x40, entrada de fora, torre verde) e no `lib/registro` (nome na ordem da seção 5, entre `FALHA_T2` e `FALHA_SD`) | FW-13 | PC | Testes no PC: `WDT` sozinho → verde; JSON/CSV com `WDT` | Concluída (07/11/2026, 3 testes novos no alarme + 1 ajustado no registro; PC 56/56) |
| 2 | Watchdog de tarefa de 2 s no `loop()`; no boot, `esp_reset_reason()` = watchdog → `WDT` **só no 1º registro**; comando de Serial `travar` (laço infinito, ferramenta de verificação) | FW-13 | 1 | `travar` → reinício em ~2 s; o 1º registro depois do boot tem `WDT` no CSV e no MQTT; o 2º, não | Concluída na placa (07/11/2026): 1ª versão disparava em 6 s e 14,6 s; com `deinit` + `init`, `travar` → `task_wdt` em 1971 ms; `WDT` só no 1º registro |
| 3 | Autoteste no `setup()` antes de liberar o `K1`: eco dos 2 MAX31865 (falhou → leitura inválida até responder), RTC (log + hora vazia, FW-08), SD montado já no `setup()` (acaba o `FALHA_SD` falso no 1º registro) | FW-14, FW-05 | 2 | Boot com cada periférico desligado: T1 → `FALHA_T1`/vermelho; T2 → `FALHA_T2`/verde; SD → `FALHA_SD`/verde; RTC → log e hora vazia; boot normal sem `FALHA_SD` no 1º registro | Concluída na placa (07/11/2026): boots com RTC e SD desligados, T1/T2 soltos em operação; RTC solto em operação → hora vazia |

### Decisões do bloco 5 (07/11/2026)
- `WDT` aparece só no 1º registro depois do reinício (evento pontual).
- Falha do RTC no autoteste: log e registros sem hora (comportamento do FW-08); a seção 5 não ganha condição nova.
- Teste do FW-13 por comando de Serial `travar`, documentado como ferramenta de verificação.

## Bloco 4 (FW-09 a FW-12: SD, MQTT, guarda e reenvio)

Data: 07/11/2026 · Base: `spec/FRS_Edge-HTST_v1.5.md`, seções 5, 6 e 7 · Plano: `docs/plano.md`, bloco 4

| # | Tarefa | Req. | Depende de | Critério de conclusão | Status |
|---|---|---|---|---|---|
| 1 | `lib/registro/`: struct do registro → linha CSV e JSON da seção 7 (campo vazio se inválida, `alarmes` com nomes separados por `\|`, `modo`, `fdv`) | FW-09, FW-10 | PC | Testes no PC reproduzem as 3 linhas de exemplo da seção 7 | Concluída (07/11/2026, 8 testes; PC 47/47) |
| 2 | `FALHA_SD` no `lib/alarme` (entrada de fora, verde) | FW-12 | 1 | Teste: cartão com falha → `FALHA_SD`, torre não muda | Concluída (07/11/2026, 3 testes; PC 50/50) |
| 3 | `seq` persistente na NVS, gravado em blocos | FW-10 | — | Reiniciar a placa: `seq` continua maior que o último, nunca repete | Concluída (07/11/2026: PC 53/53; placa: reset em `seq=92` → volta em `seq=100`) |
| 4 | SD no HSPI: 1 linha/s em `/AAAAMMDD.csv` (dia UTC) com cabeçalho; erro → `FALHA_SD` + log | FW-09, FW-12 | 1–3, módulo SD | Arquivo abre no PC com as linhas certas; tirar o cartão → `FALHA_SD`, torre igual | Concluída na placa (07/11/2026: gravação, `FALHA_SD`, retorno do cartão, `dt=1000` com o cartão removido); falta abrir o CSV no PC (sem leitor de cartão) |
| 5 | MQTT (`esp-mqtt`, QoS 1) em `htst/<device_id>/leituras`, sem bloquear; broker em `segredos.h` | FW-10 | 1, broker | `mosquitto_sub` mostra 1 JSON/s com `seq` crescente | Concluída (07/11/2026) |
| 6 | Guarda e reenvio: sem broker (ou com pendências), o JSON vai para `/PENDENTES.txt`; a tarefa do SD envia uma por vez e avança no PUBACK; posição confirmada na NVS a cada 60 s; o CSV do dia não muda | FW-11 | 4, 5 | Derrubar o broker 5 min: CSV com todas as linhas, torre segue (`dt=1000`); ao voltar, chegam todas em ordem | Concluída na placa (07/11/2026: quedas curtas do broker; 103 pendências em 4962 ms e 16 em 620 ms, `dt=1000`); queda de 5 min, conferência de ordem no consumidor e 24 h ficam na tarefa 7 |
| 7 | Bancada longa: 24 h de SD (FW-09) e queda de rede longa (FW-11) | FW-09, FW-11 | 6 | Critérios da seção 6 | Pendente |

### Pendências do bloco 4 (perguntar, não supor)
- Respondido (07/11/2026): módulo MicroSD de 3,3 V, cartão de 16 GB; broker Mosquitto no PC; `device_id` = `htst-01` (configurável); registro sem hora em `/SEMHORA.csv`.

## Bloco 3 (FW-07, FW-08: falha de sensor e hora)

Data: 06/11/2026 · Base: `spec/FRS_Edge-HTST_v1.5.md`, seções 5, 6 e 7 e Anexo A

Princípio: a regra de falha já está em `lib/alarme/` (bloco 2, tarefa 3). Aqui falta o lado do registro e a prova na bancada. A hora entra atrás de uma interface simples; a formatação ISO 8601 é lógica pura, testada no PC.

| # | Tarefa | Req. | Depende de | Critério de conclusão | Status |
|---|---|---|---|---|---|
| 1 | Leitura inválida registrada **sem valor**: no registro, T1/T2 vazios quando `t1Valida`/`t2Valida` = 0, mais as colunas de validade (como na seção 7) | FW-07 | Bloco 2 | Serial: com T2 em 319 °C, o campo de T2 sai vazio e `t2_valida=0` | Concluída (06/11/2026) |
| 2 | Bancada FW-07: PT100 aberto (bornes vazios) e em curto (jumper nos bornes), em T1 e em T2 | FW-07 | 1 | `FALHA_T1`/`FALHA_T2` em até 2 leituras; T1 → torre vermelha, T2 → verde; ao recolocar o resistor, volta | Concluída (06/11/2026) |
| 3 | `lib/hora/`: segundos UTC → texto ISO 8601 (`2026-11-02T13:05:01Z`), lógica pura | FW-08 | PC | Testes no PC: data comum, virada de dia, de mês, de ano e 29/02 | Cancelada (07/11/2026: a RTClib já formata ISO 8601; ver `decisoes.md`) |
| 4 | RTC DS3231 (I2C: SDA 21, SCL 22) atrás de uma interface (`agoraUtc()`); hora no início de cada registro; erro do RTC vira linha de log | FW-08 | 3, módulo DS3231 | Serial mostra a hora do RTC em ISO 8601; ela continua certa depois de desligar e ligar (bateria) | Concluída (07/11/2026) |
| 5 | Wi-Fi + NTP sem bloquear o loop: credenciais em `segredos.h` (fora do Git, com exemplo versionado); quando houver rede, ajusta o RTC em UTC | FW-08 | 4 | Partir sem rede → hora vem do RTC; com rede → RTC ajustado (linha de log); torre não atrasa durante a conexão | Concluída (07/11/2026) |

### Pendências do bloco 3 (perguntar, não supor)
- Tem o módulo DS3231 com bateria (CR2032)? E Wi-Fi 2,4 GHz na bancada?
- A seção 5 não tem condição para falha do RTC. Proposta: só linha de log agora; decidir no FW-14 (bloco 5) se vira condição.

## Bloco 2 (FW-03, FW-04, FW-05, FW-06: regras de alarme, relé e modo limpeza)

Data: 06/11/2026 · Base: `spec/FRS_Edge-HTST_v1.5.md`, seções 3 a 6

Princípio: as tarefas 1 a 4 são lógica pura em `firmware/lib/alarme/` (sem `Arduino.h`), testadas no PC. O módulo recebe leituras, parâmetros e o tempo (`agoraMs`) e devolve as condições ativas e a cor da torre. Só as tarefas 5 a 7 tocam no hardware.

| # | Tarefa | Req. | Depende de | Critério de conclusão | Status |
|---|---|---|---|---|---|
| 1 | Esqueleto do `lib/alarme/`: tipos de entrada (T1, T2 e validade, FDV, PRESS, modo), parâmetros (seção 4) e saída (condições ativas + torre); condições `OK` e `PRESSAO` | FW-03 | PC | Testes no PC: sem nada anormal → `OK`/verde; `PRESS` aberto → `PRESSAO`/vermelho | Concluída (06/11/2026, 3 testes) |
| 2 | `SUBTEMP` e `SUBTEMP_SEM_DESVIO`: `T1 < T_MIN`, histerese `HIST`, temporização `T_FDV` com a FDV em `DIRETO` | FW-03 | 1 | Testes: desvio funcionando → `SUBTEMP`/verde; FDV direto por ≤ `T_FDV` → verde, > `T_FDV` → vermelho; só volta com `T1 ≥ T_MIN + HIST` | Concluída (06/11/2026, 7 testes novos) |
| 3 | `FALHA_T1` e `FALHA_T2`: leitura inválida (falha do MAX31865 ou fora de `FAIXA_VALIDA`) em 2 leituras seguidas | FW-03 (FW-07 no bloco 3) | 1 | Testes: 1 leitura inválida não gera falha, 2 seguidas geram; `FALHA_T1` → vermelho, `FALHA_T2` → verde | Concluída (06/11/2026, 7 testes novos) |
| 4 | Modo limpeza: `BTN_LIMP` suspende só `SUBTEMP_SEM_DESVIO` por até `TEMPO_LIMPEZA` | FW-06 | 2 | Testes: em limpeza o cenário 3 fica verde e o 4 vermelho; após `TEMPO_LIMPEZA` o cenário 3 volta a vermelho | Concluída (06/11/2026, 7 testes novos) |
| 5 | Parâmetros da seção 4 configuráveis sem recompilar (inclui `PERIODO_REGISTRO`) | Seção 4 | — | Mudar `T_MIN` sem recompilar e ver o efeito na Serial | Concluída (06/11/2026: PC 38/38; placa: `set` gravado, `dt` seguiu o período, valores mantidos após reinício) |
| 6 | Relé `K1` (GPIO 4) com segurança positiva: desenergizado do reset até liberar a operação; energiza só com torre verde | FW-05 | Módulo relé | Cortar a alimentação → vermelho + sirene; reset → vermelho até o fim do `setup()` | Concluída (08/11/2026: relé CW-020 + BC547 com LEDs no NA/NF; desligado no reset, sem alimentação e no reinício do watchdog) |
| 7 | Integração no `main.cpp`: avaliar as regras a cada ciclo, comandar `K1`, mostrar condições na Serial; `BTN_LIMP` no Ticker com debounce | FW-03, FW-04, FW-06 | 1–6 | Cenários 1 a 7 do Quadro 1 na bancada com resistores e jumpers; tempo até a torre < 2 s | Concluída (06/11/2026: cenários 2, 3, 4 e limpeza na placa; FW-04 ≈ 0,9 s; PC 39/39; cenário 1 sem resistor adequado, 5–7 nos blocos 3 e 4) |

### Pendências que afetam o bloco 2 (perguntar, não supor)

- Resolvidas em 06/11/2026 (ver `decisoes.md`): histerese única e congelada com `T1` inválido; parâmetros na NVS por comando Serial; `BTN_LIMP` ignorado durante a limpeza.
- D1 (nível da FDV para `DESVIO`) e D2 (`T_FDV`, provisório 5 s) e D3 (`TEMPO_LIMPEZA`, provisório 4 h) entram como parâmetros.
- Hardware da bancada: módulo relé, torre (ou LEDs) e botão para as tarefas 6 e 7.

## Bloco 1 (FW-01, FW-02: leituras) — fechado em 05/11/2026

Data: 04/11/2026 · Base: `spec/FRS_Edge-HTST_v1.5.md` e `docs/plano.md`

| # | Tarefa | Req. | Depende de | Critério de conclusão | Status |
|---|---|---|---|---|---|
| 1 | Filtro de debounce como lógica pura em `lib/debounce/`, com os 6 casos de `test_debounce` (`pio test -e native`) | FW-02 | PC | Pulso < 50 ms ignorado; mudança estável reconhecida em < 200 ms | Concluída (04/11/2026, 6/6 testes) |
| 2 | Leitura de `FDV` (GPIO 27) e `PRESS` (GPIO 26) por `Ticker` a cada 10 ms com o debounce; mudanças impressas na Serial; criação do `config.h` | FW-02 | ESP32 e jumper | Na bancada, pulsos curtos ignorados e mudanças estáveis reconhecidas | Concluída (04/11/2026, jumper) |
| 3 | Interface `SensorPT100` e implementação com MAX31865; `FIOS_PT100` configurável; resolução de 0,1 °C | FW-01 | MAX31865 soldado | Leitura de T1 e T2 com resistor de teste | Concluída (05/11/2026: T1 e T2 lidas simultaneamente, 0,1 °C, sem falha) |
| 4 | Leitura periódica de T1 e T2 a cada 1 s, com log na Serial contendo `dt` | FW-01 | MAX31865 soldado | `dt` impresso em cada linha | Concluída (05/11/2026, dt = 1000 ms em todas as linhas observadas) |
| 5 | Teste de bancada de 1 h e análise do `dt` | FW-01 | MAX31865 soldado | Nenhum `dt` maior que 1 s em 1 h | Concluída (05/11/2026: 5,44 h, 19571 registros, `dt` = 1000 ms em todos) |

### Pendências que afetam o bloco 1

- D1: nível lógico do indutivo que corresponde a `DESVIO`. Na tarefa 2 imprime-se o nível cru; a tradução vira parâmetro no bloco 2.
- Número de fios do PT100 da sonda (provavelmente 3; confirmar na fábrica). Bancada com resistor: 2 fios.
