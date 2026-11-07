// Testes dos parâmetros configuráveis da seção 4 (comando "set"), no PC.
#include <unity.h>
#include "configuracao.h"

void setUp() {}
void tearDown() {}

// Seção 4: valores padrão iguais aos da spec
void test_padrao_igual_a_spec() {
  Configuracao c = configuracaoPadrao();
  TEST_ASSERT_EQUAL_FLOAT(72.0f, c.alarme.tMin);
  TEST_ASSERT_EQUAL_FLOAT(0.5f, c.alarme.hist);
  TEST_ASSERT_EQUAL_UINT32(5000, c.alarme.tFdvMs);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, c.alarme.faixaMinC);
  TEST_ASSERT_EQUAL_FLOAT(105.0f, c.alarme.faixaMaxC);
  TEST_ASSERT_EQUAL_UINT32(14400000, c.alarme.tempoLimpezaMs);
  TEST_ASSERT_EQUAL_UINT32(1000, c.periodoRegistroMs);
  TEST_ASSERT_EQUAL_UINT8(0, c.fdvNivelDesvio);
  TEST_ASSERT_TRUE(configuracaoValida(c));
}

void test_set_t_min() {
  Configuracao c = configuracaoPadrao();
  TEST_ASSERT_TRUE(aplicarSet("set T_MIN 72.5", c) == ResultadoComando::OK);
  TEST_ASSERT_EQUAL_FLOAT(72.5f, c.alarme.tMin);
}

// Tempos são digitados em segundos e guardados em ms
void test_set_tempos_em_segundos() {
  Configuracao c = configuracaoPadrao();
  TEST_ASSERT_TRUE(aplicarSet("set T_FDV 7", c) == ResultadoComando::OK);
  TEST_ASSERT_EQUAL_UINT32(7000, c.alarme.tFdvMs);
  TEST_ASSERT_TRUE(aplicarSet("set TEMPO_LIMPEZA 60", c) == ResultadoComando::OK);
  TEST_ASSERT_EQUAL_UINT32(60000, c.alarme.tempoLimpezaMs);
  TEST_ASSERT_TRUE(aplicarSet("set PERIODO_REGISTRO 0.5", c) == ResultadoComando::OK);
  TEST_ASSERT_EQUAL_UINT32(500, c.periodoRegistroMs);
}

void test_nome_desconhecido_nao_altera() {
  Configuracao c = configuracaoPadrao();
  TEST_ASSERT_TRUE(aplicarSet("set T_MAX 90", c) == ResultadoComando::NOME_DESCONHECIDO);
  TEST_ASSERT_EQUAL_FLOAT(72.0f, c.alarme.tMin);
}

// Fronteira de confiança: texto, lixo depois do número, nan e inf são recusados
void test_valor_malformado_nao_altera() {
  Configuracao c = configuracaoPadrao();
  TEST_ASSERT_TRUE(aplicarSet("set T_MIN abc", c) == ResultadoComando::VALOR_INVALIDO);
  TEST_ASSERT_TRUE(aplicarSet("set T_MIN 72abc", c) == ResultadoComando::VALOR_INVALIDO);
  TEST_ASSERT_TRUE(aplicarSet("set T_MIN nan", c) == ResultadoComando::VALOR_INVALIDO);
  TEST_ASSERT_TRUE(aplicarSet("set T_MIN inf", c) == ResultadoComando::VALOR_INVALIDO);
  TEST_ASSERT_EQUAL_FLOAT(72.0f, c.alarme.tMin);
}

void test_formato_errado() {
  Configuracao c = configuracaoPadrao();
  TEST_ASSERT_TRUE(aplicarSet("set T_MIN", c) == ResultadoComando::FORMATO_INVALIDO);
  TEST_ASSERT_TRUE(aplicarSet("T_MIN 72", c) == ResultadoComando::FORMATO_INVALIDO);
}

// Valores sem sentido são recusados e nada muda
void test_valores_sem_sentido_nao_alteram() {
  Configuracao c = configuracaoPadrao();
  TEST_ASSERT_TRUE(aplicarSet("set HIST -1", c) == ResultadoComando::VALOR_INVALIDO);
  TEST_ASSERT_TRUE(aplicarSet("set T_FDV 0", c) == ResultadoComando::VALOR_INVALIDO);
  TEST_ASSERT_TRUE(aplicarSet("set T_FDV -5", c) == ResultadoComando::VALOR_INVALIDO);
  TEST_ASSERT_TRUE(aplicarSet("set FAIXA_MIN 110", c) == ResultadoComando::VALOR_INVALIDO);  // acima do máximo
  TEST_ASSERT_EQUAL_FLOAT(0.5f, c.alarme.hist);
  TEST_ASSERT_EQUAL_UINT32(5000, c.alarme.tFdvMs);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, c.alarme.faixaMinC);
}

// D1: nível da FDV em DESVIO só aceita 0 ou 1
void test_set_fdv_desvio() {
  Configuracao c = configuracaoPadrao();
  TEST_ASSERT_TRUE(aplicarSet("set FDV_DESVIO 1", c) == ResultadoComando::OK);
  TEST_ASSERT_EQUAL_UINT8(1, c.fdvNivelDesvio);
  TEST_ASSERT_TRUE(aplicarSet("set FDV_DESVIO 2", c) == ResultadoComando::VALOR_INVALIDO);
  TEST_ASSERT_TRUE(aplicarSet("set FDV_DESVIO 0.5", c) == ResultadoComando::VALOR_INVALIDO);
  TEST_ASSERT_EQUAL_UINT8(1, c.fdvNivelDesvio);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_padrao_igual_a_spec);
  RUN_TEST(test_set_t_min);
  RUN_TEST(test_set_tempos_em_segundos);
  RUN_TEST(test_nome_desconhecido_nao_altera);
  RUN_TEST(test_valor_malformado_nao_altera);
  RUN_TEST(test_formato_errado);
  RUN_TEST(test_valores_sem_sentido_nao_alteram);
  RUN_TEST(test_set_fdv_desvio);
  return UNITY_END();
}
