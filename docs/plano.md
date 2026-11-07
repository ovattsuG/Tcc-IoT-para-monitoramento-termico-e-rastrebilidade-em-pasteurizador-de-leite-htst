# Plano — Edge-HTST (fase Plan do SDD)

Data: 03/11/2026 · Baseado em `spec/FRS_Edge-HTST_v1.5.md`

## Ferramenta e framework

| Decisão | Escolha | Alternativa descartada | Motivo |
|---|---|---|---|
| Ferramenta | VS Code + PlatformIO | Arduino IDE | Testes unitários no PC (`pio test -e native`), versão das bibliotecas registrada no `platformio.ini`, agente de IA lê `spec/` direto do projeto |
| Framework | Arduino (núcleo da Espressif para ESP32) | ESP-IDF | Mais simples, biblioteca pronta para o MAX31865, prazo curto |
| Execução | `loop()` único + `Ticker` para as entradas; **a partir do bloco 4, mais uma tarefa do FreeRTOS para o SD** (07/11/2026) | Tudo no `loop()` | Mais simples de entender; o `Ticker` resolve a leitura do PT100 (~75 ms). O SD trava até ~1,7 s quando o cartão sai, então foi para uma tarefa própria |

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
| RTClib | ^2.1.4 (07/11/2026) | DS3231 e hora ISO 8601 (FW-08) |
| SD, WiFi, esp-mqtt | do núcleo ESP32 (Arduino-ESP32 2.0.17) | Cartão, rede e MQTT (bloco 4) |
| Plataforma espressif32 | 7.1.3 (fixada após o primeiro build, 04/11/2026) | Núcleo Arduino do ESP32 |

## Testes

| Onde | O quê |
|---|---|
| PC (`pio test -e native`) | Debounce; todas as regras de alarme, incluindo a classificação da leitura |
| Bancada | Leitura com resistor, falha com fio aberto, entradas com jumper, 1 h de log (FW-01) |

## Bloco 4 — SD, MQTT, guarda e reenvio (FW-09 a FW-12) · 07/11/2026

| Decisão | Escolha | Alternativa descartada | Motivo |
|---|---|---|---|
| Cartão | Biblioteca `SD` do núcleo, no HSPI (SCK 14, MISO 32, MOSI 13, CS 15, Anexo A) | `SdFat` | Já vem no núcleo; o volume (1 linha/s) não exige mais |
| MQTT | `esp-mqtt` da Espressif (`mqtt_client.h`, já no núcleo), QoS 1, `esp_mqtt_client_enqueue` | `PubSubClient` | `PubSubClient` só publica em QoS 0 (FW-10 pede QoS 1) e bloqueia no `connect`; o `esp-mqtt` roda em tarefa própria e não segura o `loop()` |
| Fila de envio | **O próprio CSV no SD é a fila**: o envio lê a próxima linha ainda não confirmada (PUBACK) e publica; ao vivo = fila de 1 linha | Fila separada em RAM ou em outro arquivo | Um caminho só para ao vivo e reenvio; nada se perde se a rede cair (FW-11) |
| Posição confirmada | Arquivo + posição da próxima linha a enviar, gravados na NVS a cada ~60 s | Gravar a cada mensagem | 1 gravação/s gasta a flash; reiniciar reenvia até ~60 s, que o consumidor descarta por `device_id` + `seq` (FW-10) |
| `seq` | Contador na NVS gravado em blocos (ex.: a cada 100); no boot continua do bloco seguinte | Gravar a cada registro; derivar da hora | Nunca repete nem diminui; pode pular números, o que a spec não proíbe |
| Formatação | `lib/registro/` (lógica pura): linha CSV e JSON da seção 7, nomes das condições | Montar o texto no `main.cpp` | Testável no PC; CSV e JSON saem da mesma estrutura |
| SD com falha | `FALHA_SD` vira condição (verde, seção 5); o envio passa a publicar a leitura atual direto | Parar o MQTT | FW-12: monitoramento, alarme e MQTT continuam |

## Pendências (perguntar, não supor)

- D1: qual nível do indutivo e do pressostato corresponde a `DESVIO` e a "pressão OK".
- Número de fios do PT100 da sonda (provavelmente 3; confirmar na fábrica).
- Placa: ESP32 DevKit V1 genérico, 30 pinos (`esp32doit-devkit-v1`), USB CH9102.
