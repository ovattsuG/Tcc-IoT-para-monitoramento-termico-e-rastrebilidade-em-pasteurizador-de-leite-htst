// Testes do formato da seção 7 (FW-09, FW-10): as 3 linhas de exemplo da spec, o JSON e o texto que não cabe.
#include <string.h>
#include <unity.h>
#include "registro.h"

void setUp() {}
void tearDown() {}

// Linhas de exemplo da seção 7 da spec.
const Registro OK_DIRETO{10231, "2026-11-02T13:05:01Z", 75.4f, true, 33.1f, true, Fdv::DIRETO, true, false, 0};
const Registro SUBTEMP_DESVIO{10232, "2026-11-02T13:05:02Z", 71.6f, true, 33.0f, true, Fdv::DESVIO, true, false, SUBTEMP};
const Registro T1_INVALIDA{10233, "2026-11-02T13:05:03Z", 999.0f, false, 33.0f, true, Fdv::DESVIO, true, false,
                           FALHA_T1 | SUBTEMP};

void test_cabecalho_igual_ao_da_spec() {
  TEST_ASSERT_EQUAL_STRING("seq,timestamp_utc,t1_c,t2_c,t1_valida,t2_valida,fdv,pressao_ok,modo,alarmes",
                           CABECALHO_CSV);
}

void test_csv_linha_ok() {
  char linha[128];
  TEST_ASSERT_TRUE(formatarCsv(OK_DIRETO, linha, sizeof(linha)) > 0);
  TEST_ASSERT_EQUAL_STRING("10231,2026-11-02T13:05:01Z,75.4,33.1,1,1,DIRETO,1,PRODUCAO,", linha);
}

void test_csv_linha_subtemp() {
  char linha[128];
  formatarCsv(SUBTEMP_DESVIO, linha, sizeof(linha));
  TEST_ASSERT_EQUAL_STRING("10232,2026-11-02T13:05:02Z,71.6,33.0,1,1,DESVIO,1,PRODUCAO,SUBTEMP", linha);
}

// Leitura inválida: campo vazio, nunca o valor falso (999)
void test_csv_leitura_invalida_sai_vazia() {
  char linha[128];
  formatarCsv(T1_INVALIDA, linha, sizeof(linha));
  TEST_ASSERT_EQUAL_STRING("10233,2026-11-02T13:05:03Z,,33.0,0,1,DESVIO,1,PRODUCAO,FALHA_T1|SUBTEMP", linha);
}

void test_csv_limpeza_sem_hora_e_varias_condicoes() {
  Registro r = OK_DIRETO;
  r.horaUtc = "";
  r.limpeza = true;
  r.pressaoOk = false;
  r.condicoes = SUBTEMP | PRESSAO | FALHA_T2 | SUBTEMP_SEM_DESVIO;   // fora de ordem de propósito
  char linha[128];
  formatarCsv(r, linha, sizeof(linha));
  TEST_ASSERT_EQUAL_STRING("10231,,75.4,33.1,1,1,DIRETO,0,LIMPEZA,SUBTEMP_SEM_DESVIO|PRESSAO|FALHA_T2|SUBTEMP",
                           linha);
}

void test_json_com_leitura_invalida() {
  char json[256];
  TEST_ASSERT_TRUE(formatarJson(T1_INVALIDA, "htst-01", json, sizeof(json)) > 0);
  TEST_ASSERT_EQUAL_STRING(
      "{\"device_id\":\"htst-01\",\"seq\":10233,\"timestamp_utc\":\"2026-11-02T13:05:03Z\","
      "\"t1_c\":null,\"t2_c\":33.0,\"t1_valida\":0,\"t2_valida\":1,\"fdv\":\"DESVIO\","
      "\"pressao_ok\":1,\"modo\":\"PRODUCAO\",\"alarmes\":\"FALHA_T1|SUBTEMP\"}",
      json);
}

void test_json_sem_hora_e_null() {
  Registro r = OK_DIRETO;
  r.horaUtc = "";
  char json[256];
  formatarJson(r, "htst-01", json, sizeof(json));
  TEST_ASSERT_NOT_NULL(strstr(json, "\"timestamp_utc\":null,"));
}

// Todas as condições ao mesmo tempo: ordem da seção 5 e o texto cabe inteiro
void test_todas_as_condicoes_cabem_na_ordem() {
  Registro r = OK_DIRETO;
  r.condicoes = SUBTEMP | FALHA_SD | FALHA_T2 | FALHA_T1 | PRESSAO | SUBTEMP_SEM_DESVIO;
  char linha[160];
  TEST_ASSERT_TRUE(formatarCsv(r, linha, sizeof(linha)) > 0);
  TEST_ASSERT_NOT_NULL(strstr(linha, ",SUBTEMP_SEM_DESVIO|PRESSAO|FALHA_T1|FALHA_T2|FALHA_SD|SUBTEMP"));
}

// Não coube: devolve 0, e quem chama não grava nem envia
void test_texto_que_nao_cabe_devolve_zero() {
  char pequeno[20];
  TEST_ASSERT_EQUAL(0, formatarCsv(OK_DIRETO, pequeno, sizeof(pequeno)));
  TEST_ASSERT_EQUAL(0, formatarJson(OK_DIRETO, "htst-01", pequeno, sizeof(pequeno)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_cabecalho_igual_ao_da_spec);
  RUN_TEST(test_csv_linha_ok);
  RUN_TEST(test_csv_linha_subtemp);
  RUN_TEST(test_csv_leitura_invalida_sai_vazia);
  RUN_TEST(test_csv_limpeza_sem_hora_e_varias_condicoes);
  RUN_TEST(test_json_com_leitura_invalida);
  RUN_TEST(test_json_sem_hora_e_null);
  RUN_TEST(test_todas_as_condicoes_cabem_na_ordem);
  RUN_TEST(test_texto_que_nao_cabe_devolve_zero);
  return UNITY_END();
}
