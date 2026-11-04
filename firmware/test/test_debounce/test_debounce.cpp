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

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_sem_mudanca_mantem_nivel_inicial);
  RUN_TEST(test_pulso_curto_e_ignorado);
  return UNITY_END();
}
