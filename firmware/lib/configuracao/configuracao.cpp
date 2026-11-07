// Implementação dos parâmetros configuráveis (seção 4). Interface em configuracao.h.
#include "configuracao.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Configuracao configuracaoPadrao() {
  Configuracao cfg;
  cfg.alarme.tMin = 72.0f;
  cfg.alarme.hist = 0.5f;
  cfg.alarme.tFdvMs = 5000;
  cfg.alarme.faixaMinC = 0.0f;
  cfg.alarme.faixaMaxC = 105.0f;
  cfg.alarme.tempoLimpezaMs = 4UL * 60 * 60 * 1000;
  cfg.periodoRegistroMs = 1000;
  cfg.fdvNivelDesvio = 0;   // D1 provisório (06/11/2026): sensor NPN vê metal em DESVIO → pino em 0
  return cfg;
}

bool configuracaoValida(const Configuracao& c) {
  return isfinite(c.alarme.tMin) && isfinite(c.alarme.hist) && c.alarme.hist >= 0.0f &&
         isfinite(c.alarme.faixaMinC) && isfinite(c.alarme.faixaMaxC) &&
         c.alarme.faixaMinC < c.alarme.faixaMaxC &&
         c.alarme.tFdvMs > 0 && c.alarme.tempoLimpezaMs > 0 && c.periodoRegistroMs > 0 &&
         c.fdvNivelDesvio <= 1;
}

// Segundos digitados → ms. Recusa negativo e o que não cabe em uint32_t (~49 dias).
static bool segundosParaMs(float s, uint32_t& ms) {
  if (s < 0.0f || s > 4294967.0f) return false;
  ms = (uint32_t)(s * 1000.0f + 0.5f);
  return true;
}

ResultadoComando aplicarSet(const char* linha, Configuracao& cfg) {
  char nome[24];
  char valorTxt[24];
  if (sscanf(linha, "set %23s %23s", nome, valorTxt) != 2) return ResultadoComando::FORMATO_INVALIDO;

  // strtof sozinho aceitaria "72abc" como 72; exigir que o texto acabe no número.
  char* fim;
  float v = strtof(valorTxt, &fim);
  if (fim == valorTxt || *fim != '\0' || !isfinite(v)) return ResultadoComando::VALOR_INVALIDO;

  Configuracao nova = cfg;
  bool convertido = true;
  if      (strcmp(nome, "T_MIN") == 0)            nova.alarme.tMin = v;
  else if (strcmp(nome, "HIST") == 0)             nova.alarme.hist = v;
  else if (strcmp(nome, "FAIXA_MIN") == 0)        nova.alarme.faixaMinC = v;
  else if (strcmp(nome, "FAIXA_MAX") == 0)        nova.alarme.faixaMaxC = v;
  else if (strcmp(nome, "T_FDV") == 0)            convertido = segundosParaMs(v, nova.alarme.tFdvMs);
  else if (strcmp(nome, "TEMPO_LIMPEZA") == 0)    convertido = segundosParaMs(v, nova.alarme.tempoLimpezaMs);
  else if (strcmp(nome, "PERIODO_REGISTRO") == 0) convertido = segundosParaMs(v, nova.periodoRegistroMs);
  else if (strcmp(nome, "FDV_DESVIO") == 0) {
    convertido = (v == 0.0f || v == 1.0f);   // só nível 0 ou 1
    if (convertido) nova.fdvNivelDesvio = (uint8_t)v;   // converter -1 para uint8_t seria comportamento indefinido
  }
  else return ResultadoComando::NOME_DESCONHECIDO;

  if (!convertido || !configuracaoValida(nova)) return ResultadoComando::VALOR_INVALIDO;
  cfg = nova;
  return ResultadoComando::OK;
}
