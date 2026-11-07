// Regras de alarme da seção 5 da spec: recebe leituras e tempo, devolve condições ativas e cor da torre.
// FW-03, FW-06 (modo limpeza) e a classificação da leitura do FW-07. Lógica pura: não inclui Arduino.h, roda e é testada no PC.
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
  FALHA_SD           = 1 << 5,   // FW-12: só registro e envio, torre verde
  WDT                = 1 << 6,   // FW-13: reiniciou por watchdog; só registro e envio, torre verde
};

enum class Fdv : uint8_t { DIRETO, DESVIO };

// Parâmetros da seção 4 da spec.
struct Parametros {
  float tMin;        // T_MIN
  float hist;        // HIST
  uint32_t tFdvMs;   // T_FDV
  float faixaMinC;   // FAIXA_VALIDA, limite de baixo
  float faixaMaxC;   // FAIXA_VALIDA, limite de cima
  uint32_t tempoLimpezaMs;  // TEMPO_LIMPEZA
};

struct Entradas {
  bool pressaoOk;   // já traduzido do pino: true = pressostato fechado
  float t1;
  uint8_t t1Falha;  // registrador de falhas do MAX31865 #1 (0 = sem falha)
  float t2;
  uint8_t t2Falha;  // registrador de falhas do MAX31865 #2
  Fdv fdv;          // já traduzido do pino (D1)
  bool botaoLimpeza;  // BTN_LIMP já filtrado: true = apertado
  bool sdFalha;       // cartão ausente, cheio ou erro de gravação (FW-12); omitido = false
  bool reinicioWdt;   // o boot veio de um watchdog e este é o 1º registro (FW-13); omitido = false
};

struct Resultado {
  uint16_t condicoes;   // 0 = OK
  bool torreVermelha;
  bool t1Valida;        // o registro (bloco 4) usa estes, para não repetir a regra
  bool t2Valida;
  bool modoLimpeza;     // false = modo produção
};

class Alarme {
public:
  explicit Alarme(const Parametros& p);
  // Troca os limites sem perder o estado (histerese, cronômetro, limpeza): um "set" vale no próximo ciclo.
  void definirParametros(const Parametros& p) { p_ = p; }
  Resultado avaliar(const Entradas& e, uint32_t agoraMs);
private:
  bool leituraValida(float celsius, uint8_t falha) const;

  Parametros p_;
  bool subtemp_ = false;
  bool contandoDireto_ = false;
  uint32_t diretoDesdeMs_ = 0;
  uint8_t invalidasT1_ = 0;   // leituras inválidas seguidas
  uint8_t invalidasT2_ = 0;
  bool limpeza_ = false;
  // Para perceber o aperto (solto → apertado). Começa "apertado": botão preso no boot não ativa a limpeza.
  bool botaoAnterior_ = true;
  uint32_t limpezaDesdeMs_ = 0;
};
