// FW-10 / FW-11: publica o registro (JSON da seção 7) em htst/<device_id>/leituras, QoS 1, fora do loop().
// Usa o esp-mqtt do núcleo: conecta e reconecta numa tarefa própria.
#pragma once

// Chamar no setup(), depois do WiFi.begin(). Não espera o broker.
void iniciarMqtt();

// true = conectado ao broker (atualizado pela tarefa do MQTT).
bool mqttConectado();

// Envia o JSON (QoS 1). Pode esperar a rede até 5 s: chamar só da tarefa do SD, nunca do loop() (FW-04).
// Devolve o msg_id (para esperar o PUBACK com mqttConfirmou) ou -1 = desconectado ou erro (log feito).
int publicarRegistro(const char* json);

// true = o broker confirmou (PUBACK) a mensagem msgId.
bool mqttConfirmou(int msgId);
