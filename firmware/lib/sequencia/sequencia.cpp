// Implementação da reserva de "seq" (FW-10). Interface em sequencia.h.
#include "sequencia.h"

// Começa onde a reserva anterior terminou: tudo abaixo dela pode já ter sido usado antes do reinício.
Sequencia::Sequencia(uint32_t reservaGravada, uint32_t bloco)
    : atual_(reservaGravada), reserva_(reservaGravada), bloco_(bloco) {}
