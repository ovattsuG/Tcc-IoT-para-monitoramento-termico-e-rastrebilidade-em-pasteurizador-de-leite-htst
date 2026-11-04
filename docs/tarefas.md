# Tarefas — Bloco 1 (FW-01, FW-02: leituras)

Data: 04/11/2026 · Base: `spec/FRS_Edge-HTST_v1.5.md` e `docs/plano.md`

Status: Pendente · Em andamento · Implementado – aguardando bancada · Concluída

| # | Tarefa | Req. | Depende de | Critério de conclusão | Status |
|---|---|---|---|---|---|
| 1 | Filtro de debounce como lógica pura em `lib/debounce/`, com os 6 casos de `test_debounce` (`pio test -e native`) | FW-02 | PC | Pulso < 50 ms ignorado; mudança estável reconhecida em < 200 ms | Concluída (04/11/2026, 6/6 testes) |
| 2 | Leitura de `FDV` (GPIO 27) e `PRESS` (GPIO 26) por `Ticker` a cada 10 ms com o debounce; mudanças impressas na Serial; criação do `config.h` | FW-02 | ESP32 e jumper | Na bancada, pulsos curtos ignorados e mudanças estáveis reconhecidas | Pendente |
| 3 | Interface `SensorPT100` e implementação com MAX31865; `FIOS_PT100` configurável; resolução de 0,1 °C | FW-01 | MAX31865 soldado | Leitura de T1 e T2 com resistor de teste | Pendente |
| 4 | Leitura periódica de T1 e T2 a cada 1 s, com log na Serial contendo `dt` | FW-01 | MAX31865 soldado | `dt` impresso em cada linha | Pendente |
| 5 | Teste de bancada de 1 h e análise do `dt` | FW-01 | MAX31865 soldado | Nenhum `dt` maior que 1 s em 1 h | Pendente |

## Pendências que afetam o bloco

- D1: nível lógico do indutivo que corresponde a `DESVIO`. Na tarefa 2 imprime-se o nível cru; a tradução vira parâmetro no bloco 2.
- Número de fios do PT100 da sonda (provavelmente 3; confirmar na fábrica). Bancada com resistor: 2 fios.
