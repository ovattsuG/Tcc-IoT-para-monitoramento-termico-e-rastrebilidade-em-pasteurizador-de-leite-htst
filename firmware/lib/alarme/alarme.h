// Regras de alarme da seção 5 da spec: recebe leituras e tempo, devolve condições ativas e cor da torre.
// FW-03. Lógica pura: não inclui Arduino.h, roda e é testada no PC.
#pragma once
#include <stdint.h>

// Cada condição é um bit: várias podem estar ativas ao mesmo tempo.
enum Condicao : uint16_t {
  PRESSAO = 1 << 0,
};

struct Entradas {
  bool pressaoOk;   // já traduzido do pino: true = pressostato fechado
};

struct Resultado {
  uint16_t condicoes;   // 0 = OK
  bool torreVermelha;
};

class Alarme {
public:
  Resultado avaliar(const Entradas& e, uint32_t agoraMs);
};
