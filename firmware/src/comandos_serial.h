// Comandos "get" e "set NOME valor" pela Serial para ajustar os parâmetros da seção 4 sem recompilar.
// "travar": prende o loop() para o teste do watchdog (FW-13).
#pragma once
#include "configuracao.h"

void imprimirConfiguracao(const Configuracao& cfg);

// Chamar a cada volta do loop(): junta as letras que chegaram e trata a linha quando vem o Enter. Não bloqueia.
void atenderSerial(Configuracao& cfg);
