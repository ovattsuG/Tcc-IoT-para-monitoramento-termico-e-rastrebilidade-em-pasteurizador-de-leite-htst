# Tarefas

Status: Pendente · Em andamento · Implementado – aguardando bancada · Concluída

## Bloco 2 (FW-03, FW-04, FW-05, FW-06: regras de alarme, relé e modo limpeza)

Data: 06/11/2026 · Base: `spec/FRS_Edge-HTST_v1.5.md`, seções 3 a 6

Princípio: as tarefas 1 a 4 são lógica pura em `firmware/lib/alarme/` (sem `Arduino.h`), testadas no PC. O módulo recebe leituras, parâmetros e o tempo (`agoraMs`) e devolve as condições ativas e a cor da torre. Só as tarefas 5 a 7 tocam no hardware.

| # | Tarefa | Req. | Depende de | Critério de conclusão | Status |
|---|---|---|---|---|---|
| 1 | Esqueleto do `lib/alarme/`: tipos de entrada (T1, T2 e validade, FDV, PRESS, modo), parâmetros (seção 4) e saída (condições ativas + torre); condições `OK` e `PRESSAO` | FW-03 | PC | Testes no PC: sem nada anormal → `OK`/verde; `PRESS` aberto → `PRESSAO`/vermelho | Concluída (06/11/2026, 3 testes) |
| 2 | `SUBTEMP` e `SUBTEMP_SEM_DESVIO`: `T1 < T_MIN`, histerese `HIST`, temporização `T_FDV` com a FDV em `DIRETO` | FW-03 | 1 | Testes: desvio funcionando → `SUBTEMP`/verde; FDV direto por ≤ `T_FDV` → verde, > `T_FDV` → vermelho; só volta com `T1 ≥ T_MIN + HIST` | Concluída (06/11/2026, 7 testes novos) |
| 3 | `FALHA_T1` e `FALHA_T2`: leitura inválida (falha do MAX31865 ou fora de `FAIXA_VALIDA`) em 2 leituras seguidas | FW-03 (FW-07 no bloco 3) | 1 | Testes: 1 leitura inválida não gera falha, 2 seguidas geram; `FALHA_T1` → vermelho, `FALHA_T2` → verde | Pendente |
| 4 | Modo limpeza: `BTN_LIMP` suspende só `SUBTEMP_SEM_DESVIO` por até `TEMPO_LIMPEZA` | FW-06 | 2 | Testes: em limpeza o cenário 3 fica verde e o 4 vermelho; após `TEMPO_LIMPEZA` o cenário 3 volta a vermelho | Pendente |
| 5 | Parâmetros da seção 4 configuráveis sem recompilar (inclui `PERIODO_REGISTRO`) | Seção 4 | — | Mudar `T_MIN` sem recompilar e ver o efeito na Serial | Pendente |
| 6 | Relé `K1` (GPIO 4) com segurança positiva: desenergizado do reset até liberar a operação; energiza só com torre verde | FW-05 | Módulo relé | Cortar a alimentação → vermelho + sirene; reset → vermelho até o fim do `setup()` | Pendente |
| 7 | Integração no `main.cpp`: avaliar as regras a cada ciclo, comandar `K1`, mostrar condições na Serial; `BTN_LIMP` no Ticker com debounce | FW-03, FW-04, FW-06 | 1–6 | Cenários 1 a 7 do Quadro 1 na bancada com resistores e jumpers; tempo até a torre < 2 s | Pendente |

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
