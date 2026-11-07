// MicroSD no HSPI com a biblioteca SD do núcleo, numa tarefa do FreeRTOS; também envia o MQTT e as pendências.
// Interface em cartao_sd.h.
#include "cartao_sd.h"
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <string.h>
#include "config.h"
#include "parametros_nvs.h"
#include "rede_mqtt.h"
#include "registro.h"

struct ItemFila {
  char hora[24];
  char linha[160];
  char json[320];   // "" = sem JSON (não coube): só CSV
};

static const char* ARQ_PENDENTES = "/PENDENTES.txt";

static SPIClass hspi(HSPI);
static QueueHandle_t fila = nullptr;
static volatile bool falha = true;   // até montar o cartão, não há onde gravar

// Estado do reenvio (FW-11): só esta tarefa mexe nele.
static bool haPendencias = false;
static uint32_t posicao = 0;          // bytes de /PENDENTES.txt já confirmados pelo broker
static uint32_t posicaoGravada = 0;   // a que está na NVS
static uint32_t ultimaGravacaoPosMs = 0;
static int idEsperado = -1;           // msg_id aguardando PUBACK; -1 = livre
static uint32_t tamanhoEnviado = 0;   // bytes da linha em envio (inclui o '\n')
static uint32_t envioMs = 0;
static uint32_t enviadas = 0;         // confirmadas neste reenvio (log de vazão)
static uint32_t inicioReenvioMs = 0;
static bool reenviando = false;

static bool montar() {
  SD.end();
  bool ok = SD.begin(PINO_SD_CS, hspi);
  if (!ok) Serial.println("SD: cartao ausente ou ilegivel");
  return ok;
}

// Com o cartão montado, descobre se ainda há pendências depois da posição confirmada.
static void conferirPendencias() {
  uint32_t tamanho = 0;
  if (SD.exists(ARQ_PENDENTES)) {   // sem exists(), abrir arquivo inexistente gera erro na Serial
    File f = SD.open(ARQ_PENDENTES, FILE_READ);
    if (f) tamanho = f.size();
    if (f) f.close();
  }
  if (posicao > tamanho) posicao = 0;   // arquivo trocado ou cortado: recomeça (repetidas são descartadas pelo seq)
  haPendencias = tamanho > posicao;
  if (haPendencias) Serial.printf("MQTT: %u bytes pendentes no cartao\n", (unsigned)(tamanho - posicao));
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

// Só '\n' no fim (não println): o envio conta os bytes para avançar a posição.
static bool guardarPendencia(const char* json) {
  File f = SD.open(ARQ_PENDENTES, FILE_APPEND);
  bool ok = f && f.print(json) > 0 && f.print('\n') == 1;
  if (f) f.close();
  if (ok) haPendencias = true;
  else Serial.println("SD: erro ao guardar a pendencia; mensagem perdida para o MQTT");
  return ok;
}

static void gravarPosicao() {
  if (posicao == posicaoGravada) return;
  if (salvarPosicaoEnvio(posicao)) posicaoGravada = posicao;
  else Serial.println("MQTT: erro ao gravar a posicao na NVS; pode reenviar repetidas apos reinicio");
  ultimaGravacaoPosMs = millis();
}

// Ao vivo só sem pendências: quem decide é esta tarefa, então a ordem não se inverte.
// Exceção: sem cartão as pendências não podem ser lidas; publicar ao vivo é melhor que perder (o seq reordena).
static void tratarJson(const char* json, bool montado) {
  if (json[0] == '\0') return;
  if ((!haPendencias || !montado) && publicarRegistro(json) >= 0) return;
  if (montado) guardarPendencia(json);
  else Serial.println("MQTT: sem broker e sem cartao; mensagem perdida para o MQTT");
}

// Uma pendência por vez; nunca espera: ou confere o PUBACK, ou manda a próxima.
// shortcut: uma mensagem por vez (~20/s na rede local, 24 h escoam em ~1 h); se for lento demais, enviar em janela.
static void passoDeEnvio(bool montado) {
  if (idEsperado >= 0) {
    if (mqttConfirmou(idEsperado)) {
      posicao += tamanhoEnviado;
      idEsperado = -1;
      enviadas++;
    } else if (!mqttConectado() || millis() - envioMs >= TEMPO_PUBACK_MS) {
      if (mqttConectado()) Serial.printf("MQTT: sem PUBACK do id %d em %u ms; reenviando a linha da posicao %u\n",
                                         idEsperado, (unsigned)TEMPO_PUBACK_MS, (unsigned)posicao);
      idEsperado = -1;   // reenvia a mesma linha; se o broker já tinha recebido, o consumidor descarta pelo seq
    } else {
      return;
    }
  }
  if (millis() - ultimaGravacaoPosMs >= PERIODO_GRAVA_POSICAO_MS) gravarPosicao();
  if (!haPendencias || !montado || !mqttConectado()) return;

  File f = SD.open(ARQ_PENDENTES, FILE_READ);
  if (!f) {
    Serial.println("SD: erro ao abrir as pendencias");
    return;
  }
  if (posicao >= f.size()) {   // tudo confirmado: começa um arquivo novo
    f.close();
    if (!SD.remove(ARQ_PENDENTES)) Serial.println("SD: erro ao apagar as pendencias");
    posicao = 0;
    haPendencias = false;
    gravarPosicao();
    Serial.printf("MQTT: pendencias enviadas (%u em %u ms)\n",(unsigned)enviadas, (unsigned)(millis() - inicioReenvioMs));
    enviadas = 0;
    reenviando = false;
    return;
  }
  if (!reenviando) {
    reenviando = true;
    inicioReenvioMs = millis();
    Serial.printf("MQTT: reenviando a partir da posicao %u de %u bytes\n", (unsigned)posicao, (unsigned)f.size());
  }
  char json[320];
  f.seek(posicao);
  size_t n = f.readBytesUntil('\n', json, sizeof(json) - 1);
  f.close();
  json[n] = '\0';
  int id = publicarRegistro(json);
  if (id < 0) return;   // tenta de novo na próxima volta
  idEsperado = id;
  tamanhoEnviado = n + 1;
  envioMs = millis();
}

// Roda no núcleo 0, separada do loop(): pode travar nas tentativas internas da biblioteca sem afetar a torre.
static void tarefaSd(void*) {
  hspi.begin(PINO_SD_SCK, PINO_SD_MISO, PINO_SD_MOSI, PINO_SD_CS);
  posicao = posicaoGravada = carregarPosicaoEnvio();
  bool montado = montar();
  if (montado) conferirPendencias();
  uint32_t ultimaTentativaMs = millis();
  ItemFila item;
  for (;;) {
    // Espera curta durante o reenvio, mas a fila é atendida a cada volta: o CSV do dia nunca espera o MQTT.
    TickType_t espera = pdMS_TO_TICKS(haPendencias && mqttConectado() ? 20 : 1000);
    bool temItem = xQueueReceive(fila, &item, espera) == pdTRUE;
    if (!montado && millis() - ultimaTentativaMs >= PERIODO_RETENTATIVA_SD_MS) {
      ultimaTentativaMs = millis();
      montado = montar();
      if (montado) {
        Serial.println("SD: cartao de volta");
        conferirPendencias();
      }
    }
    if (temItem) {
      // shortcut: linha que chega com o cartão fora é perdida no SD; o reenvio cobre a rede, não o cartão.
      if (montado && gravar(item)) {
        falha = false;
      } else {
        falha = true;
        if (montado) {
          montado = false;
          ultimaTentativaMs = millis();
        }
      }
      tratarJson(item.json, montado);
    }
    passoDeEnvio(montado);
  }
}

void iniciarTarefaSd() {
  fila = xQueueCreate(TAMANHO_FILA_SD, sizeof(ItemFila));
  // 6 KB: além do SD, o JSON da pendência (320 B) fica na pilha.
  xTaskCreatePinnedToCore(tarefaSd, "sd", 6144, nullptr, 1, nullptr, 0);
}

bool enfileirarLinhaSd(const char* horaUtc, const char* linha, const char* json) {
  static ItemFila item;   // static: ~500 B fora da pilha do loop(); só o loop() chama
  snprintf(item.hora, sizeof(item.hora), "%s", horaUtc);
  snprintf(item.linha, sizeof(item.linha), "%s", linha);
  snprintf(item.json, sizeof(item.json), "%s", json);
  if (fila != nullptr && xQueueSend(fila, &item, 0) == pdTRUE) return true;   // 0 = não espera
  falha = true;
  return false;
}

bool sdComFalha() { return falha; }
