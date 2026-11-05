// Pinos da placa e tempos fixos do firmware.
// Mapa de pinos: spec/FRS_Edge-HTST_v1.5.md, Anexo A.
// Para mudar um valor daqui é preciso recompilar. Valores que o operador
// ajusta em campo (T_MIN, HIST...) NÃO ficam aqui (bloco 2).
#pragma once
#include <stdint.h>

constexpr uint8_t PINO_FDV   = 27;  // sensor indutivo da válvula de desvio
constexpr uint8_t PINO_PRESS = 26;  // pressostato da linha de ar

constexpr uint32_t PERIODO_AMOSTRA_MS = 10;   // de quanto em quanto tempo lê as entradas
constexpr uint32_t TEMPO_DEBOUNCE_MS  = 100;  // FW-02: tempo parado para aceitar uma mudança
