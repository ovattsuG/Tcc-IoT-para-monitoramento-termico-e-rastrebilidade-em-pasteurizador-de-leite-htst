# Matriz de verificação — requisito → teste

Cenários de bancada (TCC, Quadro 1):
1. Operação normal
2. Desvio correto
3. Leite subpasteurizado
4. Perda de ar
5. Sensor aberto
6. Sensor em curto
7. Perda de rede

Status possíveis: Pendente · Implementado – aguardando bancada · Aprovado · Reprovado.

| Req. | Critério de aceitação (resumo) | Cenário / procedimento | Status | Data | Resultado e evidência | Commit |
|---|---|---|---|---|---|---|
| FW-01 | Intervalo entre registros ≤ 1 s em 1 h | 1 h de bancada; análise do log da Serial (`dt`) e, no bloco 4, do CSV | Aprovado (Serial; CSV no bloco 4) | 05/11/2026 | Tarefa 5: log `firmware/logs/device-monitor-261105-025347.log`, 5,44 h contínuas (296000 a 19866000 ms), 19571 registros, 0 lacunas; `dt` mínimo = máximo = 1000 ms; nenhuma falha (`0x00` em todas as linhas); T1 323,9–324,3 °C e T2 22,0–22,3 °C (resistores fixos). Tarefa 3: T1 e T2 lidas com resolução de 0,1 °C, 2 fios. Placa #1: 109 Ω → 24,7 °C; 215 Ω → 319 °C; 72 Ω → −67 °C (esperado ~23 / ~308 / ~−71 °C; diferença atribuída a contatos em 2 fios e precisão do multímetro). T1 e T2 simultâneas, estáveis: `T1 = 24.5 C falha = 0x00 \| T2 = 317.8 C falha = 0x00`. Entradas FW-02 seguem respondendo durante as leituras. Resistores trocados entre as placas → leituras trocaram junto (CS corretos). Tarefa 4: registro em grade fixa de `PERIODO_REGISTRO` = 1000 ms com `dt` no log; 20 linhas observadas, todas `dt=1000` (ex.: `[36000 ms] dt=1000 \| T1 = 323.9 C ... \| T2 = 22.0 C ...`). Pendente: exatidão exige calibração com referência | |
| FW-02 | Pulso < 50 ms ignorado; mudança estável < 200 ms | Testes no PC (`test_debounce`, 6 casos) + pulsos pelo microcontrolador de teste no indutivo e no jumper | Implementado – aguardando bancada | 04/11/2026 | PC: `pio test -e native` → 6/6 casos passando (base; pulso de 40 ms ignorado; fronteira de 100 ms; reconhecimento em 100 ms, < 200 ms; tremida a cada 10 ms ignorada; queda de 90 ms ignorada). Bancada com jumper (04/11/2026): GPIO 26 e 27 com pull-up, Ticker 10 ms + filtro 100 ms; cada conexão/desconexão do jumper gerou uma única transição na Serial, sem rajadas (ex.: `[78486 ms] PRESS: 1 -> 0`, `[80506 ms] PRESS: 0 -> 1`). Pendente: pulsos < 50 ms gerados pelo microcontrolador de teste (06/11/2026: tentativa com NodeMCU ESP8266 sem sucesso na gravação; adiado, ver `decisoes.md`) | `68d80c7` (lógica) |
| FW-03 | Os 7 cenários dão o resultado esperado | Cenários 1 a 7 | Pendente | 06/11/2026 | Parcial (bloco 2, tarefa 1): `lib/alarme/` no PC, `pio test -e native` → 3 casos passando (tudo normal → `OK`/verde; pressostato aberto → `PRESSAO`/vermelho; pressão volta → verde). Tarefa 2: `SUBTEMP` (frio com desvio → verde), `SUBTEMP_SEM_DESVIO` (FDV em DIRETO por mais de `T_FDV` → vermelho; 5000 ms não dispara, 5001 dispara), histerese (72,2 mantém, 72,5 sai), T1 inválido mantém o estado, cronômetro reinicia se a FDV desvia no meio, `PRESSAO` e `SUBTEMP_SEM_DESVIO` juntas. `pio test -e native` → 16/16 (10 do alarme) | |
| FW-04 | Torre em ≤ 2 s (descontado `T_FDV`) | Cenários 3, 4 e 5, com 30 repetições cada pelos relés de teste | Pendente | | | |
| FW-05 | Sem alimentação: vermelho + sirene | Cortar a alimentação do nó | Pendente | | | |
| FW-06 | Modo limpeza suspende só o cenário 3 e expira | Cenários 3 e 4 em limpeza; esperar `TEMPO_LIMPEZA` (reduzido para o teste) | Pendente | | | |
| FW-07 | Falha de sensor detectada em ≤ 2 leituras | Cenários 5 e 6 em T1 e em T2 | Pendente | | | |
| FW-08 | Hora correta depois de partir sem rede | Desligar, religar sem Wi-Fi, comparar com a hora de referência | Pendente | | | |
| FW-09 | CSV íntegro em 24 h, inválidas marcadas | 24 h de bancada; validar o arquivo | Pendente | | | |
| FW-10 | Consumidor descarta mensagens repetidas por `device_id` + `seq` | Reenviar mensagens; conferir no banco | Pendente | | | |
| FW-11 | 24 h sem rede: sem perda, sem duplicata, FW-04 mantido | Cenário 7 | Pendente | | | |
| FW-12 | Sem SD: torre inalterada e `FALHA_SD` | Remover o cartão em operação | Pendente | | | |
| FW-13 | Travamento → reinício e `WDT` registrado | Laço infinito induzido | Pendente | | | |
| FW-14 | Periférico ausente no boot → falha sinalizada | Ligar com cada periférico desconectado | Pendente | | | |
