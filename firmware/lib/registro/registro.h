// Formato do registro da seção 7 da spec: uma linha CSV (SD, FW-09) e um JSON (MQTT, FW-10) a partir dos mesmos dados.
// Lógica pura: não inclui Arduino.h, roda e é testada no PC.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "alarme.h"

struct Registro {
  uint32_t seq;
  const char* horaUtc;   // "2026-11-02T13:05:01Z" ou "" se o RTC não tem hora confiável
  float t1;
  bool t1Valida;
  float t2;
  bool t2Valida;
  Fdv fdv;
  bool pressaoOk;
  bool limpeza;
  uint16_t condicoes;    // bits de Condicao
};

extern const char* const CABECALHO_CSV;

// Devolvem o tamanho escrito (sem o '\0'), ou 0 se não coube: nunca uma linha cortada.
size_t formatarCsv(const Registro& r, char* destino, size_t tamanho);
size_t formatarJson(const Registro& r, const char* deviceId, char* destino, size_t tamanho);
