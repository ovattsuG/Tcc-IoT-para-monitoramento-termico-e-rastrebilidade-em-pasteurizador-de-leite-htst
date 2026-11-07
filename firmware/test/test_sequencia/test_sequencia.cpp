// Testes do "seq" persistente (FW-10): crescente, grava antes de usar, nunca repete depois de reiniciar.
#include <unity.h>
#include "sequencia.h"

void setUp() {}
void tearDown() {}

// Simula a NVS: grava quando pedido e devolve o próximo número.
uint32_t nvs = 0;
uint32_t gravacoes = 0;
uint32_t proximo(Sequencia& s) {
  if (s.precisaGravar()) {
    nvs = s.reservaParaGravar();
    gravacoes++;
    s.reservaGravada();
  }
  return s.proximo();
}

void test_primeiro_boot_comeca_em_zero_e_grava_antes() {
  nvs = 0; gravacoes = 0;
  Sequencia s(nvs, 100);
  TEST_ASSERT_TRUE(s.precisaGravar());
  TEST_ASSERT_EQUAL_UINT32(0, proximo(s));
  TEST_ASSERT_EQUAL_UINT32(100, nvs);
  TEST_ASSERT_EQUAL_UINT32(1, gravacoes);
}

void test_crescente_e_grava_so_a_cada_bloco() {
  nvs = 0; gravacoes = 0;
  Sequencia s(nvs, 100);
  for (uint32_t i = 0; i < 250; i++) TEST_ASSERT_EQUAL_UINT32(i, proximo(s));
  TEST_ASSERT_EQUAL_UINT32(3, gravacoes);   // em 0, 100 e 200
  TEST_ASSERT_EQUAL_UINT32(300, nvs);
}

// Reinício no meio do bloco: continua do fim da reserva, nunca repete
void test_reinicio_nao_repete() {
  nvs = 0; gravacoes = 0;
  Sequencia antes(nvs, 100);
  uint32_t ultimo = 0;
  for (int i = 0; i < 150; i++) ultimo = proximo(antes);   // usou até 149; NVS = 200
  Sequencia depois(nvs, 100);
  uint32_t primeiro = proximo(depois);
  TEST_ASSERT_TRUE(primeiro > ultimo);
  TEST_ASSERT_EQUAL_UINT32(200, primeiro);
  TEST_ASSERT_EQUAL_UINT32(300, nvs);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_primeiro_boot_comeca_em_zero_e_grava_antes);
  RUN_TEST(test_crescente_e_grava_so_a_cada_bloco);
  RUN_TEST(test_reinicio_nao_repete);
  return UNITY_END();
}
