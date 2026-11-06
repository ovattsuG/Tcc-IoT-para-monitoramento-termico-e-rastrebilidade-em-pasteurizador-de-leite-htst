// Regras de alarme da seção 5 da spec: recebe leituras e tempo, devolve condições ativas e cor da torre.
// FW-03 (e a classificação da leitura do FW-07). Lógica pura: não inclui Arduino.h, roda e é testada no PC.
// Chamar avaliar() uma vez por ciclo de leitura: a contagem de "2 leituras seguidas" depende disso.
#pragma once
#include <stdint.h>

// Cada condição é um bit (1, 2, 4...): várias podem estar ativas ao mesmo tempo.
enum Condicao : uint16_t {
  PRESSAO            = 1 << 0,
  SUBTEMP            = 1 << 1,
  SUBTEMP_SEM_DESVIO = 1 << 2,
  FALHA_T1           = 1 << 3,
  FALHA_T2           = 1 << 4,
};

enum class Fdv : uint8_t { DIRETO, DESVIO };

// Parâmetros da seção 4 da spec.
struct Parametros {
  float tMin;        // T_MIN
  float hist;        // HIST
  uint32_t tFdvMs;   // T_FDV
  float faixaMinC;   // FAIXA_VALIDA, limite de baixo
  float faixaMaxC;   // FAIXA_VALIDA, limite de cima
};

struct Entradas {
  bool pressaoOk;   // já traduzido do pino: true = pressostato fechado
  float t1;
  uint8_t t1Falha;  // registrador de falhas do MAX31865 #1 (0 = sem falha)
  float t2;
  uint8_t t2Falha;  // registrador de falhas do MAX31865 #2
  Fdv fdv;          // já traduzido do pino (D1)
};

struct Resultado {
  uint16_t condicoes;   // 0 = OK
  bool torreVermelha;
  bool t1Valida;        // o registro (bloco 4) usa estes, para não repetir a regra
  bool t2Valida;
};

class Alarme {
public:
  explicit Alarme(const Parametros& p);
  Resultado avaliar(const Entradas& e, uint32_t agoraMs);
private:
  bool leituraValida(float celsius, uint8_t falha) const;

  Parametros p_;
  bool subtemp_ = false;
  bool contandoDireto_ = false;
  uint32_t diretoDesdeMs_ = 0;
  uint8_t invalidasT1_ = 0;   // leituras inválidas seguidas
  uint8_t invalidasT2_ = 0;
};
