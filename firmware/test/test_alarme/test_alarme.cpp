// Testes das regras de alarme da seção 5 (FW-03), no PC.
#include <unity.h>
#include "alarme.h"

// Valores da seção 4 da spec (T_FDV provisório: 5 s).
const Parametros P{72.0f, 0.5f, 5000};

// Leituras de "tudo normal"; cada teste muda só o que interessa.
Entradas normal() { return {true, 75.0f, true, Fdv::DIRETO}; }

Entradas comT1(float t1, Fdv fdv) {
  Entradas e = normal();
  e.t1 = t1;
  e.fdv = fdv;
  return e;
}

void setUp() {}
void tearDown() {}

// ---------- Tarefa 1: OK e PRESSAO ----------

// Seção 5, linha OK
void test_tudo_normal_e_ok_verde() {
  Alarme alarme(P);
  Resultado r = alarme.avaliar(normal(), 0);
  TEST_ASSERT_EQUAL_UINT16(0, r.condicoes);
  TEST_ASSERT_FALSE(r.torreVermelha);
}

// Seção 5, linha PRESSAO
void test_pressao_aberta_e_vermelho() {
  Alarme alarme(P);
  Entradas e = normal();
  e.pressaoOk = false;
  Resultado r = alarme.avaliar(e, 0);
  TEST_ASSERT_EQUAL_UINT16(PRESSAO, r.condicoes);
  TEST_ASSERT_TRUE(r.torreVermelha);
}

// Seção 5, "Retorno ao verde": a condição acaba, a torre volta sozinha
void test_pressao_volta_e_verde() {
  Alarme alarme(P);
  Entradas e = normal();
  e.pressaoOk = false;
  alarme.avaliar(e, 0);
  Resultado r = alarme.avaliar(normal(), 1000);
  TEST_ASSERT_EQUAL_UINT16(0, r.condicoes);
  TEST_ASSERT_FALSE(r.torreVermelha);
}

// ---------- Tarefa 2: SUBTEMP e SUBTEMP_SEM_DESVIO ----------

// Seção 5, linha SUBTEMP: temperatura baixa, válvula desviando → só registro
void test_subtemp_com_desvio_e_verde() {
  Alarme alarme(P);
  Resultado r = alarme.avaliar(comT1(71.5f, Fdv::DESVIO), 0);
  TEST_ASSERT_EQUAL_UINT16(SUBTEMP, r.condicoes);
  TEST_ASSERT_FALSE(r.torreVermelha);
}

// Fronteira: exatamente T_FDV em DIRETO ainda dá tempo para a válvula
void test_direto_por_exatamente_t_fdv_e_verde() {
  Alarme alarme(P);
  alarme.avaliar(comT1(71.5f, Fdv::DIRETO), 0);
  Resultado r = alarme.avaliar(comT1(71.5f, Fdv::DIRETO), 5000);
  TEST_ASSERT_EQUAL_UINT16(0, r.condicoes);
  TEST_ASSERT_FALSE(r.torreVermelha);
}

// Seção 5, linha SUBTEMP_SEM_DESVIO (caso de 13/04/2026)
void test_direto_por_mais_de_t_fdv_e_vermelho() {
  Alarme alarme(P);
  alarme.avaliar(comT1(71.5f, Fdv::DIRETO), 0);
  Resultado r = alarme.avaliar(comT1(71.5f, Fdv::DIRETO), 5001);
  TEST_ASSERT_EQUAL_UINT16(SUBTEMP_SEM_DESVIO, r.condicoes);
  TEST_ASSERT_TRUE(r.torreVermelha);
}

// Seção 4, HIST: entre 72,0 e 72,5 mantém; só sai com T1 >= 72,5
void test_histerese_so_sai_com_t_min_mais_hist() {
  Alarme alarme(P);
  alarme.avaliar(comT1(71.5f, Fdv::DESVIO), 0);
  Resultado r = alarme.avaliar(comT1(72.2f, Fdv::DESVIO), 1000);
  TEST_ASSERT_EQUAL_UINT16(SUBTEMP, r.condicoes);
  r = alarme.avaliar(comT1(72.5f, Fdv::DESVIO), 2000);
  TEST_ASSERT_EQUAL_UINT16(0, r.condicoes);
}

// Decisão de 06/11/2026: com T1 inválido, a subtemperatura fica no último valor
void test_t1_invalido_mantem_subtemp() {
  Alarme alarme(P);
  alarme.avaliar(comT1(71.5f, Fdv::DESVIO), 0);
  Entradas e = comT1(0.0f, Fdv::DESVIO);
  e.t1Valida = false;
  Resultado r = alarme.avaliar(e, 1000);
  TEST_ASSERT_EQUAL_UINT16(SUBTEMP, r.condicoes);
}

// "DIRETO por mais de T_FDV" é tempo contínuo: um desvio no meio zera o cronômetro
void test_voltar_a_direto_reinicia_cronometro() {
  Alarme alarme(P);
  alarme.avaliar(comT1(71.5f, Fdv::DIRETO), 0);
  alarme.avaliar(comT1(71.5f, Fdv::DESVIO), 3000);
  alarme.avaliar(comT1(71.5f, Fdv::DIRETO), 4000);
  Resultado r = alarme.avaliar(comT1(71.5f, Fdv::DIRETO), 9000);   // 5 s desde 4000
  TEST_ASSERT_EQUAL_UINT16(0, r.condicoes);
  r = alarme.avaliar(comT1(71.5f, Fdv::DIRETO), 9001);
  TEST_ASSERT_EQUAL_UINT16(SUBTEMP_SEM_DESVIO, r.condicoes);
}

// Seção 5: "todas as condições ativas são registradas"
void test_pressao_e_subtemp_sem_desvio_juntas() {
  Alarme alarme(P);
  Entradas e = comT1(71.5f, Fdv::DIRETO);
  e.pressaoOk = false;
  alarme.avaliar(e, 0);
  Resultado r = alarme.avaliar(e, 6000);
  TEST_ASSERT_EQUAL_UINT16(PRESSAO | SUBTEMP_SEM_DESVIO, r.condicoes);
  TEST_ASSERT_TRUE(r.torreVermelha);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_tudo_normal_e_ok_verde);
  RUN_TEST(test_pressao_aberta_e_vermelho);
  RUN_TEST(test_pressao_volta_e_verde);
  RUN_TEST(test_subtemp_com_desvio_e_verde);
  RUN_TEST(test_direto_por_exatamente_t_fdv_e_verde);
  RUN_TEST(test_direto_por_mais_de_t_fdv_e_vermelho);
  RUN_TEST(test_histerese_so_sai_com_t_min_mais_hist);
  RUN_TEST(test_t1_invalido_mantem_subtemp);
  RUN_TEST(test_voltar_a_direto_reinicia_cronometro);
  RUN_TEST(test_pressao_e_subtemp_sem_desvio_juntas);
  return UNITY_END();
}
