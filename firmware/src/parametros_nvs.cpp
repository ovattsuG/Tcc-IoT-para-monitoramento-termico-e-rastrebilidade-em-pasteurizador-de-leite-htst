// Configuracao na NVS com a biblioteca Preferences (núcleo ESP32). Interface em parametros_nvs.h.
#include "parametros_nvs.h"
#include <Arduino.h>
#include <Preferences.h>

static const char* ESPACO = "edge-htst";   // "pasta" dentro da NVS (máximo 15 letras)
static const char* CHAVE = "cfg";

void carregarConfiguracao(Configuracao& cfg) {
  Preferences prefs;
  if (!prefs.begin(ESPACO, false)) {   // false = leitura e escrita: cria a "pasta" na primeira vez
    Serial.println("config: erro ao abrir a NVS; usando valores padrao");
    return;
  }
  if (!prefs.isKey(CHAVE)) {
    Serial.println("config: nada gravado; usando valores padrao");
    prefs.end();
    return;
  }
  // shortcut: grava a struct inteira em bytes; se um firmware novo mudar a struct, o tamanho muda,
  // a leitura é recusada e voltam os padrões (o operador precisa refazer os "set").
  Configuracao lida;
  bool ok = prefs.getBytesLength(CHAVE) == sizeof(lida) &&
            prefs.getBytes(CHAVE, &lida, sizeof(lida)) == sizeof(lida) &&
            configuracaoValida(lida);
  prefs.end();
  if (ok) {
    cfg = lida;
    Serial.println("config: carregada da NVS");
  } else {
    Serial.println("config: valor gravado invalido; usando valores padrao");
  }
}

static const char* CHAVE_SEQ = "seq";

uint32_t carregarReservaSeq() {
  Preferences prefs;
  if (!prefs.begin(ESPACO, false)) {
    Serial.println("seq: erro ao abrir a NVS; comecando de 0");
    return 0;
  }
  uint32_t reserva = prefs.getUInt(CHAVE_SEQ, 0);
  prefs.end();
  return reserva;
}

bool salvarReservaSeq(uint32_t reserva) {
  Preferences prefs;
  if (!prefs.begin(ESPACO, false)) return false;
  bool ok = prefs.putUInt(CHAVE_SEQ, reserva) == sizeof(reserva);
  prefs.end();
  return ok;
}

bool salvarConfiguracao(const Configuracao& cfg) {
  Preferences prefs;
  if (!prefs.begin(ESPACO, false)) return false;
  bool ok = prefs.putBytes(CHAVE, &cfg, sizeof(cfg)) == sizeof(cfg);
  prefs.end();
  return ok;
}
