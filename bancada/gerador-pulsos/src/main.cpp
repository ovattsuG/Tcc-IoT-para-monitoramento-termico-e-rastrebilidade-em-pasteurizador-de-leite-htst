// Gerador de pulsos para o teste de bancada do FW-02 (roda no NodeMCU de teste, não no produto).
// Ciclo: 10 pulsos de 40 ms (devem ser ignorados) + 1 nível estável de 300 ms (deve ser reconhecido).
#include <Arduino.h>

constexpr uint8_t PINO_SAIDA = 5;  // D1 do NodeMCU

void pulso(uint32_t duracaoMs) {
  digitalWrite(PINO_SAIDA, LOW);   // LOW = "contato fechou" para a entrada com pull-up da ESP32
  digitalWrite(LED_BUILTIN, LOW);  // LED do NodeMCU acende em LOW: mostra o pulso a olho
  delay(duracaoMs);
  digitalWrite(PINO_SAIDA, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
}

void setup() {
  pinMode(PINO_SAIDA, OUTPUT);
  digitalWrite(PINO_SAIDA, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  for (int i = 0; i < 10; i++) {
    pulso(40);
    delay(1000);
  }
  pulso(300);
  delay(3000);
}
