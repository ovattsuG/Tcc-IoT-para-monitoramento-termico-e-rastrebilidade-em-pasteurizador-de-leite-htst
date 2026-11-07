// Parâmetros da seção 4 da spec, configuráveis sem recompilar: valores padrão e o comando "set NOME valor".
// Seção 4 (usada por FW-01, FW-03, FW-06). Lógica pura: a gravação na NVS fica em src/.
#pragma once
#include <stdint.h>
#include "alarme.h"

struct Configuracao {
  Parametros alarme;
  uint32_t periodoRegistroMs;   // PERIODO_REGISTRO
  uint8_t fdvNivelDesvio;       // D1: nível do pino da FDV em DESVIO (0 ou 1)
};

enum class ResultadoComando { OK, FORMATO_INVALIDO, NOME_DESCONHECIDO, VALOR_INVALIDO };

// Valores da seção 4 da spec (os provisórios: T_FDV 5 s, TEMPO_LIMPEZA 4 h; D1: DESVIO = 0).
Configuracao configuracaoPadrao();

// Recusa o que não faz sentido (HIST negativo, tempo zero, faixa invertida...). Usada também ao carregar da NVS.
bool configuracaoValida(const Configuracao& cfg);

// Interpreta "set NOME valor" (tempos em segundos). Só altera cfg se o comando inteiro for válido.
ResultadoComando aplicarSet(const char* linha, Configuracao& cfg);
