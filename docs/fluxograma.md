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

Ainda não existe (próximos blocos): regras de alarme e torre (bloco 2), tratamento da falha de sensor e hora (bloco 3), SD e MQTT (bloco 4), watchdog e autoteste (bloco 5).
