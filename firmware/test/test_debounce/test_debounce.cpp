#include <unity.h>
#include "debounce.h"

void setUp() {}
void tearDown() {}

void test_sem_mudanca_mantem_nivel_inicial() {
  Debounce d(100, false);
  for (uint32_t t = 0; t <= 500; t += 10) {
    TEST_ASSERT_FALSE(d.atualiza(false, t));
  }
}

void test_pulso_curto_e_ignorado() {
  Debounce d(100, false);
  d.atualiza(false, 0);
  TEST_ASSERT_FALSE(d.atualiza(true, 10));   // pulso começa
  TEST_ASSERT_FALSE(d.atualiza(true, 40));   // 30 ms de pulso
  TEST_ASSERT_FALSE(d.atualiza(false, 50));  // pulso de 40 ms termina
  TEST_ASSERT_FALSE(d.atualiza(false, 200)); // continua baixo
}

void test_mudanca_estavel_e_reconhecida() {
  Debounce d(100, false);
  d.atualiza(false, 0);
  TEST_ASSERT_FALSE(d.atualiza(true, 10));   // sinal muda
  TEST_ASSERT_FALSE(d.atualiza(true, 100));  // 90 ms: ainda não
  TEST_ASSERT_TRUE(d.atualiza(true, 110));   // 100 ms: reconhece
}

void test_reconhece_entre_100_e_200_ms() {
  Debounce d(100, false);
  const uint32_t mudanca = 1000;
  uint32_t reconhecidoEm = 0;
  for (uint32_t t = mudanca; t <= mudanca + 300; t += 10) {
    if (d.atualiza(true, t)) {
      reconhecidoEm = t;
      break;
    }
  }
  TEST_ASSERT_TRUE(reconhecidoEm > 0);
  uint32_t atraso = reconhecidoEm - mudanca;
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(100, atraso);
  TEST_ASSERT_LESS_THAN_UINT32(200, atraso);  // limite do FW-02
}

void test_tremida_nao_muda_nivel() {
  Debounce d(100, false);
  bool nivel = false;
  for (uint32_t t = 0; t <= 300; t += 10) {
    nivel = !nivel;  // alterna a cada leitura
    TEST_ASSERT_FALSE(d.atualiza(nivel, t));
  }
}

void test_queda_de_90_ms_e_ignorada() {
  Debounce d(100, true);
  d.atualiza(true, 0);
  for (uint32_t t = 10; t <= 100; t += 10) {  // baixo de 10 a 100 ms
    TEST_ASSERT_TRUE(d.atualiza(false, t));
  }
  TEST_ASSERT_TRUE(d.atualiza(true, 110));  // volta antes de 100 ms estável
  TEST_ASSERT_TRUE(d.atualiza(true, 500));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_sem_mudanca_mantem_nivel_inicial);
  RUN_TEST(test_pulso_curto_e_ignorado);
  RUN_TEST(test_mudanca_estavel_e_reconhecida);
  RUN_TEST(test_reconhece_entre_100_e_200_ms);
  RUN_TEST(test_tremida_nao_muda_nivel);
  RUN_TEST(test_queda_de_90_ms_e_ignorada);
  return UNITY_END();
}
