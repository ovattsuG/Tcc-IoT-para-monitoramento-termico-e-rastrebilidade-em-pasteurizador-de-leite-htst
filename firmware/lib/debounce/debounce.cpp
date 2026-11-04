#include "debounce.h"

Debounce::Debounce(uint32_t tempoEstavelMs, bool nivelInicial)
    : tempoEstavelMs_(tempoEstavelMs),
      estavel_(nivelInicial),
      candidato_(nivelInicial),
      desdeMs_(0) {}

bool Debounce::atualiza(bool nivelCru, uint32_t agoraMs) {
  if (nivelCru != candidato_) {
    candidato_ = nivelCru;
    desdeMs_ = agoraMs;
  }
  if (candidato_ != estavel_ && (agoraMs - desdeMs_) >= tempoEstavelMs_) {
    estavel_ = candidato_;
  }
  return estavel_;
}
