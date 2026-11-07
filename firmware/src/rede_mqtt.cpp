// Cliente MQTT (esp-mqtt, núcleo ESP32). Interface em rede_mqtt.h.
#include "rede_mqtt.h"
#include <Arduino.h>
#include <mqtt_client.h>
#include <string.h>
#include "segredos.h"

static esp_mqtt_client_handle_t cliente = nullptr;
static volatile bool conectado = false;
static volatile int ultimoIdConfirmado = -1;   // msg_id do último PUBACK (FW-11)
static char topico[64];

// Roda na tarefa do MQTT, não no loop(): só atualiza a flag e avisa.
static void aoEventoMqtt(void*, esp_event_base_t, int32_t id, void* dados) {
  if (id == MQTT_EVENT_CONNECTED) {
    conectado = true;
    Serial.println("MQTT: conectado ao broker");
  } else if (id == MQTT_EVENT_DISCONNECTED) {
    if (conectado) Serial.println("MQTT: desconectado; tentando de novo");
    conectado = false;
  } else if (id == MQTT_EVENT_PUBLISHED) {
    ultimoIdConfirmado = static_cast<esp_mqtt_event_handle_t>(dados)->msg_id;
  }
}

void iniciarMqtt() {
  snprintf(topico, sizeof(topico), "htst/%s/leituras", DEVICE_ID);
  esp_mqtt_client_config_t config = {};
  config.uri = MQTT_URI;
  config.client_id = DEVICE_ID;
  config.buffer_size = 512;   // o JSON da seção 7 tem até ~300 letras
  // publish espera a rede até este tempo; 5 s cabe na fila de 10 linhas do SD (10 s), então o CSV não perde linha.
  config.network_timeout_ms = 5000;
  cliente = esp_mqtt_client_init(&config);
  if (cliente == nullptr) {
    Serial.println("MQTT: erro ao criar o cliente");
    return;
  }
  esp_mqtt_client_register_event(cliente, MQTT_EVENT_ANY, aoEventoMqtt, nullptr);
  if (esp_mqtt_client_start(cliente) != ESP_OK) Serial.println("MQTT: erro ao iniciar o cliente");
}

bool mqttConectado() { return conectado; }

int publicarRegistro(const char* json) {
  if (!conectado) return -1;
  // publish (não enqueue): o enqueue só sai na volta seguinte da tarefa do MQTT (~1 s), e o reenvio
  // ficava em ~1 mensagem/s, sem nunca alcançar o ao vivo. Escreve no socket na hora; quem chama é a tarefa do SD.
  int id = esp_mqtt_client_publish(cliente, topico, json, 0, 1, 0);
  if (id < 0) {
    Serial.println("MQTT: erro ao enfileirar a mensagem");
    return -1;
  }
  return id;
}

// Só uma pendência espera PUBACK por vez (o ao vivo para enquanto há pendências), então basta o último id.
bool mqttConfirmou(int msgId) { return msgId >= 0 && ultimoIdConfirmado == msgId; }
