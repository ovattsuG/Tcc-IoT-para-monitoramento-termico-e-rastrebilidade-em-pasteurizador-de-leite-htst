#include "debounce.h"

Debounce::Debounce(uint32_t tempoEstavelMs, bool nivelInicial)
    : tempoEstavelMs_(tempoEstavelMs),
      estavel_(nivelInicial),
      candidato_(nivelInicial),
      desdeMs_(0) {}

bool Debounce::atualiza(bool nivelCru, uint32_t agoraMs) {
  // Cada oscilação zera o cronômetro: ruído nunca fica parado tempo suficiente.
  if (nivelCru != candidato_) {
    candidato_ = nivelCru;
    desdeMs_ = agoraMs;
  }
  // Subtração sem sinal continua correta quando millis() volta a zero (~49 dias).
  if (candidato_ != estavel_ && (agoraMs - desdeMs_) >= tempoEstavelMs_) {
    estavel_ = candidato_;
  }
  return estavel_;
}
