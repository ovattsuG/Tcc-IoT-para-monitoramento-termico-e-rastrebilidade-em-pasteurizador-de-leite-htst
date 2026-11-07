// FW-10: publica o registro (JSON da seção 7) em htst/<device_id>/leituras, QoS 1, sem bloquear o loop().
// Usa o esp-mqtt do núcleo: conecta e reconecta numa tarefa própria.
#pragma once

// Chamar no setup(), depois do WiFi.begin(). Não espera o broker.
void iniciarMqtt();

// true = conectado ao broker (atualizado pela tarefa do MQTT).
bool mqttConectado();

// Põe o JSON na caixa de saída do cliente (QoS 1) e volta na hora. false = desconectado ou erro (log feito).
bool publicarRegistro(const char* json);
