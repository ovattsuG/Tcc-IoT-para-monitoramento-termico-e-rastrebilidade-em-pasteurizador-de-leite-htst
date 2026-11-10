# Fluxograma do firmware

Atualizado a cada bloco fechado. Base: `spec/FRS_Edge-HTST_v1.5.md`.

## Bloco 1 — leituras (FW-01, FW-02) · 05/11/2026

```mermaid
flowchart TD
    subgraph Setup["setup() — uma vez ao ligar"]
        S1[Serial 115200] --> S2[FDV GPIO 27 e PRESS GPIO 26<br/>INPUT_PULLUP]
        S2 --> S3[Ticker a cada 10 ms → lerEntradas]
        S3 --> S4{T1 CS 5 e T2 CS 17<br/>respondem ao eco SPI?}
        S4 -- não --> S5[Serial: Tn: MAX31865 nao responde]
        S4 -- sim --> L0
        S5 --> L0
    end

    subgraph Ticker["Ticker — a cada 10 ms (FW-02)"]
        T1[agora = millis] --> T2[digitalRead FDV e PRESS<br/>nível cru]
        T2 --> T3[Debounce: aceita só<br/>após 100 ms estável]
        T3 --> T4[(fdv / press<br/>volatile)]
    end

    subgraph Loop["loop() — o tempo todo"]
        L0([início]) --> L1{fdv ou press<br/>mudou?}
        L1 -- sim --> L2[Serial: FDV/PRESS a → b]
        L1 -- não --> L3
        L2 --> L3{agora - marca<br/>≥ PERIODO_REGISTRO?}
        L3 -- não --> L0
        L3 -- sim --> L4[marca += período<br/>dt = agora - último]
        L4 --> L5[ler T1 e T2<br/>MAX31865, ~75 ms cada]
        L5 --> L6[Serial: agora, dt, T1, falha, T2, falha]
        L6 --> L0
    end

    T4 -. lido por .-> L1
```

Por que dois caminhos: a leitura do PT100 trava ~150 ms por ciclo; as entradas ficam no Ticker para continuarem sendo amostradas a cada 10 ms nesse intervalo.

(Os blocos 2 e 3 estão no diagrama abaixo.)

## Blocos 2 e 3 — alarme, torre, limpeza, falha de sensor e hora (FW-03 a FW-08) · 07/11/2026

O bloco 2 não teve fechamento próprio (pendências de bancada: FW-05 com relé + BC547, FW-04 com 30 repetições); o diagrama abaixo mostra o firmware como está ao fim do bloco 3.

```mermaid
flowchart TD
    subgraph Setup["setup() — uma vez ao ligar"]
        S0[K1 = LOW antes de virar saída<br/>FW-05: torre vermelha] --> S1[Serial 115200]
        S1 --> S2[carrega Configuracao da NVS<br/>ou padrões da seção 4]
        S2 --> S3[FDV 27 e PRESS 26 INPUT_PULLUP<br/>BTN_LIMP 34 INPUT, pull-up externo]
        S3 --> S4[Ticker 10 ms → lerEntradas]
        S4 --> S5[MAX31865 T1 e T2: eco SPI<br/>falha → log]
        S5 --> S6{RTC DS3231?}
        S6 -- não responde --> S7[log; registro sem hora]
        S6 -- perdeu a hora --> S7
        S6 -- ok --> S8[horaOk = true]
        S7 --> S9
        S8 --> S9[WiFi.begin + configTime UTC<br/>não espera]
        S9 --> L0
    end

    subgraph Ticker["Ticker — a cada 10 ms (FW-02)"]
        T1[digitalRead FDV, PRESS, BTN] --> T2[Debounce 100 ms] --> T3[(fdv / press / btn<br/>volatile)]
    end

    subgraph Rede["Tarefa de rede (2º plano)"]
        N1[Wi-Fi conecta e reconecta sozinho] --> N2[SNTP acerta a hora] --> N3[(ntpSincronizou = true)]
    end

    subgraph Loop["loop() — o tempo todo"]
        L0([início]) --> L1[atenderSerial: get / set → valida → grava NVS → aplica]
        L1 --> L2{ntpSincronizou?}
        L2 -- sim --> L3[rtc.adjust; horaOk = true; log]
        L2 -- não --> L4
        L3 --> L4[foto de fdv, press, btn<br/>log de mudança]
        L4 --> L5{passou 1 s?<br/>PERIODO_LEITURA_MS}
        L5 -- não --> L9
        L5 -- sim --> L6[lê T1 e T2 ~150 ms]
        L6 --> L7[Entradas: pressaoOk = !press<br/>fdv vs FDV_DESVIO, botão = !btn]
        L7 --> L8[definirParametros + avaliar<br/>lib/alarme]
        L8 --> L8b[K1 = verde ? HIGH : LOW<br/>log se condições mudaram]
        L8b --> L9{passou PERIODO_REGISTRO?}
        L9 -- não --> L0
        L9 -- sim --> L10[linha: hora UTC ou vazio, dt,<br/>T vazio se inválida, *_valida, falha, cond]
        L10 --> L0
    end

    subgraph Alarme["lib/alarme — avaliar() (lógica pura, 39 testes no PC)"]
        A1[limpeza: aperto solto→apertado,<br/>expira em TEMPO_LIMPEZA] --> A2[PRESSAO se sem ar]
        A2 --> A3[validade: falha do chip ou fora da FAIXA_VALIDA<br/>2 seguidas → FALHA_T1 / FALHA_T2]
        A3 --> A4[histerese SUBTEMP só com T1 válido]
        A4 --> A5[SUBTEMP se DESVIO;<br/>DIRETO > T_FDV → SUBTEMP_SEM_DESVIO<br/>suspenso em limpeza]
        A5 --> A6[vermelho = PRESSAO, SUBTEMP_SEM_DESVIO ou FALHA_T1]
    end

    T3 -. lido por .-> L4
    N3 -. lido por .-> L2
    L8 -. chama .-> A1
```

Três "linhas de execução": o Ticker (entradas a cada 10 ms), a tarefa de rede do ESP32 (Wi-Fi e NTP) e o `loop()`. As duas primeiras só deixam valores prontos (`volatile`); quem decide e mexe no hardware (K1, RTC) é sempre o `loop()`.

Ainda não existe: SD, MQTT e reenvio (bloco 4), watchdog e autoteste (bloco 5).


## Blocos 4 e 5 — SD, MQTT, reenvio, watchdog e autoteste (FW-09 a FW-14) · 07/11/2026

O bloco 4 ainda não foi fechado (falta a tarefa 7: 24 h com SD e queda longa de rede); o diagrama mostra o firmware como está ao fim do bloco 5.

```mermaid
flowchart TD
    subgraph Setup["setup() — uma vez ao ligar (K1 desligado o tempo todo)"]
        S0[K1 = LOW antes de virar saída] --> S1[Serial; motivo do reinício<br/>watchdog → wdtPendente]
        S1 --> S2[Configuracao e reserva do seq da NVS]
        S2 --> S3[entradas + Ticker 10 ms]
        S3 --> S4[autoteste: eco T1 e T2, RTC, ...]
        S4 --> S5[Wi-Fi, NTP e MQTT em 2º plano]
        S5 --> S6[monta o cartão, confere /PENDENTES.txt<br/>cria a fila e a tarefa do SD]
        S6 --> S7[linha autoteste: T1 / T2 / RTC / SD]
        S7 --> S8[watchdog 2 s: tira IDLE0, deinit, init,<br/>inscreve o loop, devolve IDLE0]
    end

    subgraph Loop["loop() — núcleo 1"]
        L0([esp_task_wdt_reset]) --> L1[Serial: get / set / travar]
        L1 --> L2[NTP → RTC]
        L2 --> L3{passou 1 s?}
        L3 -- sim --> L4[lê T1, T2<br/>sem eco → falha 0xFF]
        L4 --> L5[Entradas + sdFalha + reinicioWdt]
        L5 --> L6[lib/alarme avaliar]
        L6 --> L7{1ª avaliação?}
        L7 -- sim --> L8[torre vermelha]
        L7 -- não --> L9[K1 segue o alarme]
        L8 --> L10
        L9 --> L10{passou PERIODO_REGISTRO?}
        L3 -- não --> L10
        L10 -- sim --> L11[seq; hora do RTC se isValid, senão vazia]
        L11 --> L12[CSV + JSON → fila do SD, não espera<br/>entregue → wdtPendente = false]
        L12 --> L0
        L10 -- não --> L0
    end

    subgraph SD["tarefa do SD — núcleo 0"]
        D1[recebe da fila] --> D2[grava o CSV do dia<br/>erro → FALHA_SD]
        D2 --> D3{pendências ou sem broker?}
        D3 -- não --> D4[publish ao vivo QoS 1]
        D3 -- sim --> D5[fim de /PENDENTES.txt]
        D6[passoDeEnvio: uma por vez,<br/>avança no PUBACK, posição na NVS]
    end

    subgraph WDT["watchdog de tarefas"]
        W1{loop sem reset por 2 s?} -- sim --> W2[pânico → reinício<br/>K1 desligado]
    end

    L12 -. fila .-> D1
    L0 -. alimenta .-> W1
    W2 -. boot .-> S0
```

Quatro "linhas de execução": Ticker (entradas), rede (Wi-Fi, NTP, MQTT), tarefa do SD (cartão e envio) e `loop()` (regras e torre). Só o `loop()` mexe no K1; tudo que pode esperar (cartão, rede) está fora dele, e o watchdog garante que, se o `loop()` parar, o nó reinicia com a torre vermelha.
