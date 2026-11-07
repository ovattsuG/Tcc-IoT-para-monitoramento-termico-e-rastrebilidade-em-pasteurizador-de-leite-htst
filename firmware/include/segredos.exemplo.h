// Modelo das credenciais e endereços (FW-08, FW-10). Copie para segredos.h (fora do Git) e preencha.
#pragma once

constexpr const char* WIFI_SSID  = "nome-da-rede";
constexpr const char* WIFI_SENHA = "senha-da-rede";

// Broker MQTT da bancada (Mosquitto no PC): IP do PC na rede Wi-Fi.
constexpr const char* MQTT_URI  = "mqtt://192.168.0.10:1883";
// Identidade do nó no tópico (htst/<DEVICE_ID>/leituras) e no JSON.
constexpr const char* DEVICE_ID = "htst-01";
