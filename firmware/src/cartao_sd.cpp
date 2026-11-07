// MicroSD no HSPI com a biblioteca SD do núcleo, numa tarefa do FreeRTOS. Interface em cartao_sd.h.
#include "cartao_sd.h"
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <string.h>
#include "config.h"
#include "registro.h"

struct ItemFila {
  char hora[24];
  char linha[160];
};

static SPIClass hspi(HSPI);
static QueueHandle_t fila = nullptr;
static volatile bool falha = true;   // até montar o cartão, não há onde gravar

static bool montar() {
  SD.end();
  bool ok = SD.begin(PINO_SD_CS, hspi);
  if (!ok) Serial.println("SD: cartao ausente ou ilegivel");
  return ok;
}

// "2026-11-07T04:19:30Z" → "/20261107.csv"
static void nomeArquivo(const char* horaUtc, char (&nome)[16]) {
  if (horaUtc[0] == '\0') {
    snprintf(nome, sizeof(nome), "/SEMHORA.csv");
    return;
  }
  snprintf(nome, sizeof(nome), "/%.4s%.2s%.2s.csv", horaUtc, horaUtc + 5, horaUtc + 8);
}

// Abre, grava e fecha a cada linha: fechar garante que a linha está no cartão (falta de energia, cartão removido).
static bool gravar(const ItemFila& item) {
  char nome[16];
  nomeArquivo(item.hora, nome);
  File f = SD.open(nome, FILE_APPEND);
  bool ok = f;
  if (ok && f.size() == 0) ok = f.println(CABECALHO_CSV) > 0;
  if (ok) ok = f.println(item.linha) > 0;
  if (f) f.close();
  if (!ok) Serial.printf("SD: erro ao gravar em %s\n", nome);
  return ok;
}

// Roda no núcleo 0, separada do loop(): pode travar nas tentativas internas da biblioteca sem afetar a torre.
static void tarefaSd(void*) {
  hspi.begin(PINO_SD_SCK, PINO_SD_MISO, PINO_SD_MOSI, PINO_SD_CS);
  bool montado = montar();
  uint32_t ultimaTentativaMs = millis();
  ItemFila item;
  for (;;) {
    bool temItem = xQueueReceive(fila, &item, pdMS_TO_TICKS(1000)) == pdTRUE;
    if (!montado && millis() - ultimaTentativaMs >= PERIODO_RETENTATIVA_SD_MS) {
      ultimaTentativaMs = millis();
      montado = montar();
      if (montado) Serial.println("SD: cartao de volta");
    }
    if (!temItem) continue;
    // shortcut: linha que chega com o cartão fora é perdida no SD; o reenvio por MQTT (tarefa 6) cobre a rede, não o cartão.
    if (montado && gravar(item)) {
      falha = false;
    } else {
      falha = true;
      if (montado) {
        montado = false;
        ultimaTentativaMs = millis();
      }
    }
  }
}

void iniciarTarefaSd() {
  fila = xQueueCreate(TAMANHO_FILA_SD, sizeof(ItemFila));
  xTaskCreatePinnedToCore(tarefaSd, "sd", 4096, nullptr, 1, nullptr, 0);
}

bool enfileirarLinhaSd(const char* horaUtc, const char* linha) {
  ItemFila item;
  snprintf(item.hora, sizeof(item.hora), "%s", horaUtc);
  snprintf(item.linha, sizeof(item.linha), "%s", linha);
  if (fila != nullptr && xQueueSend(fila, &item, 0) == pdTRUE) return true;   // 0 = não espera
  falha = true;
  return false;
}

bool sdComFalha() { return falha; }
