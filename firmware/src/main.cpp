// FW-02: lê a válvula de desvio (FDV) e o pressostato (PRESS) a cada 10 ms,
// filtra o ruído de contato e mostra na Serial cada mudança aceita.
#include <Arduino.h>
#include <Ticker.h>
#include "config.h"
#include "debounce.h"

Ticker tickerEntradas;
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
  Serial.begin(115200);
  pinMode(PINO_FDV, INPUT_PULLUP);
  pinMode(PINO_PRESS, INPUT_PULLUP);
  tickerEntradas.attach_ms(PERIODO_AMOSTRA_MS, lerEntradas);
}

// Último valor impresso, para o loop() só escrever quando algo mudar.
bool fdvAnterior = true;
bool pressAnterior = true;

void loop() {
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
}
