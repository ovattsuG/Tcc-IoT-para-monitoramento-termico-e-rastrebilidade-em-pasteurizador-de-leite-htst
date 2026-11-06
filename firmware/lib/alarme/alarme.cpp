// Implementação das regras de alarme (FW-03). Interface em alarme.h.
#include "alarme.h"

Resultado Alarme::avaliar(const Entradas& e, uint32_t agoraMs) {
  (void)agoraMs;  // passa a ser usado na tarefa 2 (T_FDV)
  Resultado r{0, false};
  if (!e.pressaoOk) r.condicoes |= PRESSAO;
  // Só as condições críticas da seção 5 deixam a torre vermelha.
  r.torreVermelha = (r.condicoes & PRESSAO) != 0;
  return r;
}
