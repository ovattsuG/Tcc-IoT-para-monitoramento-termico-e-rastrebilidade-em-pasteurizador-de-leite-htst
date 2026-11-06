# Plano — Edge-HTST (fase Plan do SDD)

Data: 03/11/2026 · Baseado em `spec/FRS_Edge-HTST_v1.5.md`

## Ferramenta e framework

| Decisão | Escolha | Alternativa descartada | Motivo |
|---|---|---|---|
| Ferramenta | VS Code + PlatformIO | Arduino IDE | Testes unitários no PC (`pio test -e native`), versão das bibliotecas registrada no `platformio.ini`, agente de IA lê `spec/` direto do projeto |
| Framework | Arduino (núcleo da Espressif para ESP32) | ESP-IDF | Mais simples, biblioteca pronta para o MAX31865, prazo curto |
| Execução | `loop()` único + `Ticker` para as entradas | Tarefas do FreeRTOS | Mais simples de entender; o `Ticker` resolve o único ponto crítico do bloco 1 (leitura do PT100 bloqueia ~75 ms) |

## Estrutura

```
firmware/            projeto PlatformIO (platformio.ini aqui)
  src/                 código que depende da placa
    main.cpp           setup/loop, agenda as leituras
    config.h           pinos e parâmetros
    temperatura.*      SensorPT100: MAX31865 atrás de uma interface simples
  lib/                 lógica pura, sem hardware (testada no PC)
    debounce/          FW-02: filtro das entradas discretas
    alarme/            FW-03: regras da seção 5; inclui a classificação da leitura (FW-07, parte)
  test/                testes Unity, ambiente native
spec/  docs/         na raiz
```

Regra: o que está em `lib/` não inclui `Arduino.h`. Assim roda no PC.

## Bibliotecas

| Biblioteca | Versão | Uso |
|---|---|---|
| Adafruit MAX31865 library | ^1.6.2 (instalada 1.6.2 em 05/11/2026) | Leitura dos PT100 |
| Adafruit BusIO | 1.17.4, instalada automaticamente como dependência | SPI da biblioteca acima |
| SPI, Wire, Ticker | do núcleo ESP32 | Barramentos e temporizador; `Wire` também servirá ao DS3231 |
| Plataforma espressif32 | 7.1.3 (fixada após o primeiro build, 04/11/2026) | Núcleo Arduino do ESP32 |

## Testes

| Onde | O quê |
|---|---|
| PC (`pio test -e native`) | Debounce; todas as regras de alarme, incluindo a classificação da leitura |
| Bancada | Leitura com resistor, falha com fio aberto, entradas com jumper, 1 h de log (FW-01) |

## Pendências (perguntar, não supor)

- D1: qual nível do indutivo e do pressostato corresponde a `DESVIO` e a "pressão OK".
- Número de fios do PT100 da sonda (provavelmente 3; confirmar na fábrica).
- Placa: ESP32 DevKit V1 genérico, 30 pinos (`esp32doit-devkit-v1`), USB CH9102.
