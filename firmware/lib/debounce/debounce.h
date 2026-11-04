#pragma once
#include <stdint.h>

class Debounce {
public:
  Debounce(uint32_t tempoEstavelMs, bool nivelInicial);
  bool atualiza(bool nivelCru, uint32_t agoraMs);

private:
  uint32_t tempoEstavelMs_;
  bool estavel_;      // nível que o resto do programa enxerga
  bool candidato_;    // último nível cru visto
  uint32_t desdeMs_;  // desde quando o candidato é o mesmo
};
