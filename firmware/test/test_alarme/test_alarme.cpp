// Testes das regras de alarme da seção 5 (FW-03), no PC.
#include <unity.h>
#include "alarme.h"

// Valores da seção 4 da spec (T_FDV provisório: 5 s; FAIXA_VALIDA 0 a 105 °C).
const Parametros P{72.0f, 0.5f, 5000, 0.0f, 105.0f};

// Qualquer valor diferente de 0 no registrador de falhas do MAX31865.
const uint8_t FALHA_CHIP = 0x80;

// Leituras de "tudo normal"; cada teste muda só o que interessa.
Entradas normal() { return {true, 75.0f, 0, 33.0f, 0, Fdv::DIRETO}; }

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
  e.t1Falha = FALHA_CHIP;   // 1 leitura inválida: congela, mas ainda não é FALHA_T1
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

// ---------- Tarefa 3: FALHA_T1 e FALHA_T2 ----------

// Uma leitura ruim sozinha não basta ("2 leituras seguidas")
void test_uma_leitura_invalida_nao_gera_falha() {
  Alarme alarme(P);
  Entradas e = normal();
  e.t1Falha = FALHA_CHIP;
  Resultado r = alarme.avaliar(e, 0);
  TEST_ASSERT_FALSE(r.t1Valida);
  TEST_ASSERT_EQUAL_UINT16(0, r.condicoes);
  TEST_ASSERT_FALSE(r.torreVermelha);
}

// Seção 5, linha FALHA_T1: sem T1 não há como verificar a pasteurização
void test_duas_invalidas_seguidas_geram_falha_t1_vermelho() {
  Alarme alarme(P);
  Entradas e = normal();
  e.t1Falha = FALHA_CHIP;
  alarme.avaliar(e, 0);
  Resultado r = alarme.avaliar(e, 1000);
  TEST_ASSERT_EQUAL_UINT16(FALHA_T1, r.condicoes);
  TEST_ASSERT_TRUE(r.torreVermelha);
}

// "Seguidas": uma leitura boa no meio zera a contagem
void test_invalida_valida_invalida_nao_gera_falha() {
  Alarme alarme(P);
  Entradas ruim = normal();
  ruim.t1Falha = FALHA_CHIP;
  alarme.avaliar(ruim, 0);
  alarme.avaliar(normal(), 1000);
  Resultado r = alarme.avaliar(ruim, 2000);
  TEST_ASSERT_EQUAL_UINT16(0, r.condicoes);
}

// FAIXA_VALIDA: acima de 105 °C conta como inválida mesmo sem falha do chip
void test_fora_da_faixa_gera_falha_t1() {
  Alarme alarme(P);
  Entradas e = normal();
  e.t1 = 105.1f;
  alarme.avaliar(e, 0);
  Resultado r = alarme.avaliar(e, 1000);
  TEST_ASSERT_FALSE(r.t1Valida);
  TEST_ASSERT_EQUAL_UINT16(FALHA_T1, r.condicoes);
}

// Fronteira da FAIXA_VALIDA: 0 e 105 °C ainda são válidos
void test_limites_da_faixa_sao_validos() {
  Alarme alarme(P);
  Entradas e = normal();
  e.t2 = 0.0f;
  alarme.avaliar(e, 0);
  Resultado r = alarme.avaliar(e, 1000);
  TEST_ASSERT_TRUE(r.t2Valida);
  e.t2 = 105.0f;
  r = alarme.avaliar(e, 2000);
  TEST_ASSERT_TRUE(r.t2Valida);
  TEST_ASSERT_EQUAL_UINT16(0, r.condicoes);
}

// Seção 5, linha FALHA_T2: só registro, torre verde
void test_falha_t2_e_verde() {
  Alarme alarme(P);
  Entradas e = normal();
  e.t2Falha = FALHA_CHIP;
  alarme.avaliar(e, 0);
  Resultado r = alarme.avaliar(e, 1000);
  TEST_ASSERT_EQUAL_UINT16(FALHA_T2, r.condicoes);
  TEST_ASSERT_FALSE(r.torreVermelha);
}

// Retorno ao verde: sensor volta, falha termina (e a contagem não "dá a volta" com o tempo)
void test_falha_t1_longa_termina_quando_sensor_volta() {
  Alarme alarme(P);
  Entradas e = normal();
  e.t1Falha = FALHA_CHIP;
  Resultado r{};
  for (uint32_t i = 0; i < 300; i++) r = alarme.avaliar(e, i * 1000);
  TEST_ASSERT_EQUAL_UINT16(FALHA_T1, r.condicoes);   // ainda em falha após 300 leituras
  r = alarme.avaliar(normal(), 300000);
  TEST_ASSERT_EQUAL_UINT16(0, r.condicoes);
  TEST_ASSERT_FALSE(r.torreVermelha);
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
  RUN_TEST(test_uma_leitura_invalida_nao_gera_falha);
  RUN_TEST(test_duas_invalidas_seguidas_geram_falha_t1_vermelho);
  RUN_TEST(test_invalida_valida_invalida_nao_gera_falha);
  RUN_TEST(test_fora_da_faixa_gera_falha_t1);
  RUN_TEST(test_limites_da_faixa_sao_validos);
  RUN_TEST(test_falha_t2_e_verde);
  RUN_TEST(test_falha_t1_longa_termina_quando_sensor_volta);
  return UNITY_END();
}
