// Grava e carrega a Configuracao (seção 4 da spec) na NVS do ESP32, para os parâmetros sobreviverem ao desligamento.
#pragma once
#include "configuracao.h"

// Se não houver nada gravado, ou o gravado for inválido, mantém cfg como está (padrão) e avisa na Serial.
void carregarConfiguracao(Configuracao& cfg);

// false = a NVS falhou; quem chamou não deve aplicar o valor novo (o que roda tem que ser o que está gravado).
bool salvarConfiguracao(const Configuracao& cfg);

// FW-10: fim da reserva de "seq" (ver lib/sequencia). 0 se nada gravado ou a NVS falhou (com log).
uint32_t carregarReservaSeq();
bool salvarReservaSeq(uint32_t reserva);

// FW-11: bytes de /PENDENTES.txt já confirmados pelo broker. 0 se nada gravado ou a NVS falhou (com log).
uint32_t carregarPosicaoEnvio();
bool salvarPosicaoEnvio(uint32_t posicao);
