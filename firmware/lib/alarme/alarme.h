// Regras de alarme da seção 5 da spec: recebe leituras e tempo, devolve condições ativas e cor da torre.
// FW-03. Lógica pura: não inclui Arduino.h, roda e é testada no PC.
#pragma once
#include <stdint.h>

// Cada condição é um bit (1, 2, 4...): várias podem estar ativas ao mesmo tempo.
enum Condicao : uint16_t {
  PRESSAO            = 1 << 0,
  SUBTEMP            = 1 << 1,
  SUBTEMP_SEM_DESVIO = 1 << 2,
};

enum class Fdv : uint8_t { DIRETO, DESVIO };

// Parâmetros da seção 4 da spec.
struct Parametros {
  float tMin;        // T_MIN
  float hist;        // HIST
  uint32_t tFdvMs;   // T_FDV
};

struct Entradas {
  bool pressaoOk;   // já traduzido do pino: true = pressostato fechado
  float t1;
  bool t1Valida;
  Fdv fdv;          // já traduzido do pino (D1)
};

struct Resultado {
  uint16_t condicoes;   // 0 = OK
  bool torreVermelha;
};

class Alarme {
public:
  explicit Alarme(const Parametros& p);
  Resultado avaliar(const Entradas& e, uint32_t agoraMs);
private:
  Parametros p_;
  bool subtemp_ = false;
  bool contandoDireto_ = false;
  uint32_t diretoDesdeMs_ = 0;
};
