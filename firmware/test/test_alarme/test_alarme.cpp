// Testes das regras de alarme da seção 5 (FW-03), no PC.
#include <unity.h>
#include "alarme.h"

void setUp() {}
void tearDown() {}

// Seção 5, linha OK
void test_tudo_normal_e_ok_verde() {
  Alarme alarme;
  Resultado r = alarme.avaliar({true}, 0);
  TEST_ASSERT_EQUAL_UINT16(0, r.condicoes);
  TEST_ASSERT_FALSE(r.torreVermelha);
}

// Seção 5, linha PRESSAO
void test_pressao_aberta_e_vermelho() {
  Alarme alarme;
  Resultado r = alarme.avaliar({false}, 0);
  TEST_ASSERT_EQUAL_UINT16(PRESSAO, r.condicoes);
  TEST_ASSERT_TRUE(r.torreVermelha);
}

// Seção 5, "Retorno ao verde": a condição acaba, a torre volta sozinha
void test_pressao_volta_e_verde() {
  Alarme alarme;
  alarme.avaliar({false}, 0);
  Resultado r = alarme.avaliar({true}, 1000);
  TEST_ASSERT_EQUAL_UINT16(0, r.condicoes);
  TEST_ASSERT_FALSE(r.torreVermelha);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_tudo_normal_e_ok_verde);
  RUN_TEST(test_pressao_aberta_e_vermelho);
  RUN_TEST(test_pressao_volta_e_verde);
  return UNITY_END();
}
