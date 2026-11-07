// Implementação do formato da seção 7. Interface em registro.h.
#include "registro.h"
#include <stdio.h>
#include <string.h>

const char* const CABECALHO_CSV =
    "seq,timestamp_utc,t1_c,t2_c,t1_valida,t2_valida,fdv,pressao_ok,modo,alarmes";

// Ordem da tabela da seção 5 (a mesma do exemplo "FALHA_T1|SUBTEMP").
static const struct { uint16_t bit; const char* nome; } NOMES[] = {
    {SUBTEMP_SEM_DESVIO, "SUBTEMP_SEM_DESVIO"},
    {PRESSAO, "PRESSAO"},
    {FALHA_T1, "FALHA_T1"},
    {FALHA_T2, "FALHA_T2"},
    {WDT, "WDT"},
    {FALHA_SD, "FALHA_SD"},
    {SUBTEMP, "SUBTEMP"},
};

// "FALHA_T1|SUBTEMP"; vazio = OK. Os 7 nomes, com os "|", somam 65 letras: cabem em 80.
static void nomesCondicoes(uint16_t condicoes, char (&texto)[80]) {
  texto[0] = '\0';
  for (const auto& n : NOMES) {
    if (!(condicoes & n.bit)) continue;
    if (texto[0] != '\0') strcat(texto, "|");
    strcat(texto, n.nome);
  }
}

// Temperatura válida → "75.4"; inválida → vazio (CSV) ou "null" (JSON).
static void temperatura(float c, bool valida, const char* seInvalida, char (&texto)[16]) {
  if (valida) snprintf(texto, sizeof(texto), "%.1f", c);
  else snprintf(texto, sizeof(texto), "%s", seInvalida);
}

static size_t resultado(int n, size_t tamanho) {
  return (n < 0 || (size_t)n >= tamanho) ? 0 : (size_t)n;
}

static const char* textoFdv(Fdv f) { return f == Fdv::DESVIO ? "DESVIO" : "DIRETO"; }
static const char* textoModo(bool limpeza) { return limpeza ? "LIMPEZA" : "PRODUCAO"; }

size_t formatarCsv(const Registro& r, char* destino, size_t tamanho) {
  char t1[16], t2[16], alarmes[80];
  temperatura(r.t1, r.t1Valida, "", t1);
  temperatura(r.t2, r.t2Valida, "", t2);
  nomesCondicoes(r.condicoes, alarmes);
  int n = snprintf(destino, tamanho, "%lu,%s,%s,%s,%d,%d,%s,%d,%s,%s", (unsigned long)r.seq,
                   r.horaUtc, t1, t2, r.t1Valida, r.t2Valida, textoFdv(r.fdv), r.pressaoOk,
                   textoModo(r.limpeza), alarmes);
  return resultado(n, tamanho);
}

size_t formatarJson(const Registro& r, const char* deviceId, char* destino, size_t tamanho) {
  char t1[16], t2[16], alarmes[80], hora[32];
  temperatura(r.t1, r.t1Valida, "null", t1);
  temperatura(r.t2, r.t2Valida, "null", t2);
  nomesCondicoes(r.condicoes, alarmes);
  // Sem hora: null em vez de "" (o consumidor distingue "sem valor" de texto vazio).
  if (r.horaUtc[0] != '\0') snprintf(hora, sizeof(hora), "\"%s\"", r.horaUtc);
  else snprintf(hora, sizeof(hora), "null");
  int n = snprintf(destino, tamanho,
                   "{\"device_id\":\"%s\",\"seq\":%lu,\"timestamp_utc\":%s,\"t1_c\":%s,\"t2_c\":%s,"
                   "\"t1_valida\":%d,\"t2_valida\":%d,\"fdv\":\"%s\",\"pressao_ok\":%d,"
                   "\"modo\":\"%s\",\"alarmes\":\"%s\"}",
                   deviceId, (unsigned long)r.seq, hora, t1, t2, r.t1Valida, r.t2Valida,
                   textoFdv(r.fdv), r.pressaoOk, textoModo(r.limpeza), alarmes);
  return resultado(n, tamanho);
}
