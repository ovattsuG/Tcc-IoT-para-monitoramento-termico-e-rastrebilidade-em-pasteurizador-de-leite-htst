// Cliente MQTT (esp-mqtt, núcleo ESP32). Interface em rede_mqtt.h.
#include "rede_mqtt.h"
#include <Arduino.h>
#include <mqtt_client.h>
#include <string.h>
#include "segredos.h"

static esp_mqtt_client_handle_t cliente = nullptr;
static volatile bool conectado = false;
static char topico[64];

// Roda na tarefa do MQTT, não no loop(): só atualiza a flag e avisa.
static void aoEventoMqtt(void*, esp_event_base_t, int32_t id, void*) {
  if (id == MQTT_EVENT_CONNECTED) {
    conectado = true;
    Serial.println("MQTT: conectado ao broker");
  } else if (id == MQTT_EVENT_DISCONNECTED) {
    if (conectado) Serial.println("MQTT: desconectado; tentando de novo");
    conectado = false;
  }
}

void iniciarMqtt() {
  snprintf(topico, sizeof(topico), "htst/%s/leituras", DEVICE_ID);
  esp_mqtt_client_config_t config = {};
  config.uri = MQTT_URI;
  config.client_id = DEVICE_ID;
  config.buffer_size = 512;   // o JSON da seção 7 tem até ~300 letras
  cliente = esp_mqtt_client_init(&config);
  if (cliente == nullptr) {
    Serial.println("MQTT: erro ao criar o cliente");
    return;
  }
  esp_mqtt_client_register_event(cliente, MQTT_EVENT_ANY, aoEventoMqtt, nullptr);
  if (esp_mqtt_client_start(cliente) != ESP_OK) Serial.println("MQTT: erro ao iniciar o cliente");
}

bool mqttConectado() { return conectado; }

bool publicarRegistro(const char* json) {
  if (!conectado) return false;
  // enqueue: a tarefa do MQTT envia e reenvia até o PUBACK; o loop() não espera a rede.
  int id = esp_mqtt_client_enqueue(cliente, topico, json, 0, 1, 0, true);
  if (id < 0) {
    Serial.println("MQTT: erro ao enfileirar a mensagem");
    return false;
  }
  return true;
}
