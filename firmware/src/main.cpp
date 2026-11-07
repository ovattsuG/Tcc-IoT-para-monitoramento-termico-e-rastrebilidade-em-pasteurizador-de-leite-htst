// FW-02: lê a válvula de desvio (FDV) e o pressostato (PRESS) a cada 10 ms,
// filtra o ruído de contato e mostra na Serial cada mudança aceita.
// FW-01: lê as temperaturas T1 e T2 (PT100 + MAX31865) e mostra na Serial.
// Seção 4: parâmetros carregados da NVS e ajustáveis pela Serial (get / set NOME valor).
// FW-05: relé K1 da torre desenergizado do reset até o fim do setup().
#include <Arduino.h>
#include <Ticker.h>
#include "config.h"
#include "debounce.h"
#include "temperatura.h"
#include "configuracao.h"
#include "parametros_nvs.h"
#include "comandos_serial.h"

Ticker tickerEntradas;
Configuracao cfg = configuracaoPadrao();
SensorPT100 sensorT1(PINO_CS_T1, FIOS_PT100);
SensorPT100 sensorT2(PINO_CS_T2, FIOS_PT100);

// Um filtro por entrada. Começam em 1: com pull-up, pino solto lê 1.
Debounce filtroFdv(TEMPO_DEBOUNCE_MS, true);
Debounce filtroPress(TEMPO_DEBOUNCE_MS, true);

// Valores já filtrados. volatile: escritos pelo Ticker e lidos pelo loop().
volatile bool fdv = true;
volatile bool press = true;

// Chamada pelo Ticker a cada 10 ms
void lerEntradas() {
  uint32_t agora = millis();
  fdv = filtroFdv.atualiza(digitalRead(PINO_FDV), agora);
  press = filtroPress.atualiza(digitalRead(PINO_PRESS), agora);
}

void setup() {
  // FW-05: K1 desenergizado antes de qualquer outra coisa; valor escrito antes de virar saída.
  digitalWrite(PINO_K1, LOW);
  pinMode(PINO_K1, OUTPUT);

  Serial.begin(115200);
  carregarConfiguracao(cfg);
  imprimirConfiguracao(cfg);

  pinMode(PINO_FDV, INPUT_PULLUP);
  pinMode(PINO_PRESS, INPUT_PULLUP);
  tickerEntradas.attach_ms(PERIODO_AMOSTRA_MS, lerEntradas);

  if (!sensorT1.iniciar()) {
    Serial.println("T1: MAX31865 nao responde");
  }
  if (!sensorT2.iniciar()) {
    Serial.println("T2: MAX31865 nao responde");
  }

  // Provisório (tarefa 6): fim do setup libera a operação. Na tarefa 7 quem decide é o Alarme.
  Serial.println("setup ok: K1 energizado");
  digitalWrite(PINO_K1, HIGH);
}

// Último valor impresso, para o loop() só escrever quando algo mudar.
bool fdvAnterior = true;
bool pressAnterior = true;

uint32_t marcaMs = 0;            // marca da grade: 1000, 2000, 3000...
uint32_t ultimoRegistroMs = 0;   // quando saiu a última linha, para o dt

void loop() {
  atenderSerial(cfg);

  // Foto dos valores: o Ticker pode trocá-los no meio deste trecho.
  bool fdvAgora = fdv;
  bool pressAgora = press;

  if (fdvAgora != fdvAnterior) {
    Serial.printf("[%lu ms] FDV: %d -> %d\n", millis(), fdvAnterior, fdvAgora);
    fdvAnterior = fdvAgora;
  }
  if (pressAgora != pressAnterior) {
    Serial.printf("[%lu ms] PRESS: %d -> %d\n", millis(), pressAnterior, pressAgora);
    pressAnterior = pressAgora;
  }

  uint32_t agora = millis();
  if (agora - marcaMs >= cfg.periodoRegistroMs) {
    marcaMs += cfg.periodoRegistroMs;  // grade fixa: atraso não se acumula
    uint32_t dt = agora - ultimoRegistroMs;
    ultimoRegistroMs = agora;

    LeituraTemperatura t1 = sensorT1.ler();
    LeituraTemperatura t2 = sensorT2.ler();
    Serial.printf("[%lu ms] dt=%lu | T1 = %.1f C  falha = 0x%02X | T2 = %.1f C  falha = 0x%02X\n",
                  agora, dt, t1.celsius, t1.falha, t2.celsius, t2.falha);
  }
}
