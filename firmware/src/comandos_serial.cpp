// Leitura dos comandos da Serial e resposta ao operador. Interface em comandos_serial.h.
#include "comandos_serial.h"
#include <Arduino.h>
#include <string.h>
#include "parametros_nvs.h"

void imprimirConfiguracao(const Configuracao& c) {
  Serial.printf("T_MIN=%.2f C  HIST=%.2f C  FAIXA_MIN=%.1f C  FAIXA_MAX=%.1f C\n",
                c.alarme.tMin, c.alarme.hist, c.alarme.faixaMinC, c.alarme.faixaMaxC);
  Serial.printf("T_FDV=%.3f s  TEMPO_LIMPEZA=%.0f s  PERIODO_REGISTRO=%.3f s  FDV_DESVIO=%u\n",
                c.alarme.tFdvMs / 1000.0, c.alarme.tempoLimpezaMs / 1000.0,
                c.periodoRegistroMs / 1000.0, c.fdvNivelDesvio);
  Serial.printf("OFFSET_T1=%.1f C  OFFSET_T2=%.1f C\n", c.offsetT1C, c.offsetT2C);
}

static const char* textoErro(ResultadoComando r) {
  switch (r) {
    case ResultadoComando::FORMATO_INVALIDO:  return "use: get  ou  set NOME valor";
    case ResultadoComando::NOME_DESCONHECIDO: return "nome desconhecido";
    case ResultadoComando::VALOR_INVALIDO:    return "valor invalido";
    default:                                  return "";
  }
}

static void tratarLinha(const char* linha, Configuracao& cfg) {
  // FW-13: teste de bancada do watchdog; prende o loop() sem ceder a vez, como um travamento de verdade.
  if (strcmp(linha, "travar") == 0) {
    Serial.printf("[%lu ms] travar: loop() preso; o watchdog deve reiniciar o no em ~2 s\n", millis());
    Serial.flush();
    volatile bool preso = true;   // volatile: o compilador não pode apagar o laço
    while (preso) {
    }
  }
  if (strcmp(linha, "get") == 0) {
    imprimirConfiguracao(cfg);
    return;
  }
  Configuracao nova = cfg;
  ResultadoComando r = aplicarSet(linha, nova);
  if (r != ResultadoComando::OK) {
    Serial.printf("erro: %s (\"%s\"); nada mudou\n", textoErro(r), linha);
    return;
  }
  if (!salvarConfiguracao(nova)) {
    Serial.println("erro: falha ao gravar na NVS; nada mudou");
    return;
  }
  cfg = nova;
  Serial.printf("ok: %s (gravado)\n", linha);
  imprimirConfiguracao(cfg);
}

void atenderSerial(Configuracao& cfg) {
  static char linha[48];
  static size_t tam = 0;
  static bool longaDemais = false;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;   // o monitor pode mandar \r\n no Enter
    if (c != '\n') {
      if (tam < sizeof(linha) - 1) linha[tam++] = c;
      else longaDemais = true;
      continue;
    }
    linha[tam] = '\0';
    // Uma linha cortada pode virar outro comando válido; por isso é recusada inteira.
    if (longaDemais) Serial.println("erro: linha longa demais; nada mudou");
    else if (linha[0] != '\0') tratarLinha(linha, cfg);
    tam = 0;
    longaDemais = false;
  }
}
