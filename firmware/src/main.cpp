// FW-02: lê a válvula de desvio (FDV), o pressostato (PRESS) e o BTN_LIMP a cada 10 ms, com filtro de ruído.
// FW-01: lê T1 e T2 (PT100 + MAX31865) a cada 1 s. FW-03/04/06: avalia as regras da seção 5 e comanda a torre.
// FW-05: relé K1 desenergizado do reset até a primeira avaliação verde.
// Seção 4: parâmetros carregados da NVS e ajustáveis pela Serial (get / set NOME valor).
#include <Arduino.h>
#include <Ticker.h>
#include "config.h"
#include "debounce.h"
#include "temperatura.h"
#include "configuracao.h"
#include "parametros_nvs.h"
#include "comandos_serial.h"
#include "alarme.h"
#include <RTClib.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include "segredos.h"   // fora do Git; modelo em include/segredos.exemplo.h

Ticker tickerEntradas;
Configuracao cfg = configuracaoPadrao();
SensorPT100 sensorT1(PINO_CS_T1, FIOS_PT100);
SensorPT100 sensorT2(PINO_CS_T2, FIOS_PT100);
Alarme alarme(cfg.alarme);
RTC_DS3231 rtc;
bool rtcPresente = false;
bool horaOk = false;   // FW-08: false = RTC não responde ou perdeu a hora; o registro sai sem hora

// FW-08: o SNTP chama isto na tarefa de rede, não no loop(); por isso só marca, e o loop() grava no RTC.
volatile bool ntpSincronizou = false;
void aoSincronizarNtp(struct timeval*) { ntpSincronizou = true; }

// Um filtro por entrada. Começam em 1: com pull-up, pino solto lê 1.
Debounce filtroFdv(TEMPO_DEBOUNCE_MS, true);
Debounce filtroPress(TEMPO_DEBOUNCE_MS, true);
Debounce filtroBtn(TEMPO_DEBOUNCE_MS, true);

// Valores já filtrados. volatile: escritos pelo Ticker e lidos pelo loop().
volatile bool fdv = true;
volatile bool press = true;
volatile bool btn = true;

// Chamada pelo Ticker a cada 10 ms
void lerEntradas() {
  uint32_t agora = millis();
  fdv = filtroFdv.atualiza(digitalRead(PINO_FDV), agora);
  press = filtroPress.atualiza(digitalRead(PINO_PRESS), agora);
  btn = filtroBtn.atualiza(digitalRead(PINO_BTN_LIMP), agora);
}

void setup() {
  // FW-05: K1 desenergizado antes de qualquer outra coisa; valor escrito antes de virar saída.
  digitalWrite(PINO_K1, LOW);
  pinMode(PINO_K1, OUTPUT);

  Serial.begin(115200);
  carregarConfiguracao(cfg);
  imprimirConfiguracao(cfg);

  pinMode(PINO_FDV, INPUT_PULLUP);
  pinMode(PINO_PRESS, INPUT_PULLUP);
  pinMode(PINO_BTN_LIMP, INPUT);   // GPIO 34 não tem pull-up interno: o de 10 kΩ é externo
  tickerEntradas.attach_ms(PERIODO_AMOSTRA_MS, lerEntradas);

  if (!sensorT1.iniciar()) {
    Serial.println("T1: MAX31865 nao responde");
  }
  if (!sensorT2.iniciar()) {
    Serial.println("T2: MAX31865 nao responde");
  }
  // FW-08: I2C padrão do ESP32 (SDA 21, SCL 22).
  rtcPresente = rtc.begin();
  if (!rtcPresente) {
    Serial.println("RTC: DS3231 nao responde; registros sem hora");
  } else if (rtc.lostPower()) {
    Serial.println("RTC: perdeu a hora (bateria); registros sem hora ate o ajuste por NTP");
  } else {
    horaOk = true;
  }

  // FW-08: não espera a rede; sem Wi-Fi o nó opera com a hora do RTC e tenta de novo sozinho.
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_SENHA);
  sntp_set_time_sync_notification_cb(aoSincronizarNtp);
  configTime(0, 0, "pool.ntp.org");   // fuso 0 = UTC
  Serial.println("setup ok: K1 espera a primeira avaliacao");
}

// Último valor impresso, para o loop() só escrever quando algo mudar.
bool fdvAnterior = true;
bool pressAnterior = true;
// Começa como "vermelho": o K1 está desligado até a primeira avaliação.
Resultado anterior{0, true, false, false, false};

// Última leitura e último resultado, para o registro imprimir no período dele.
LeituraTemperatura t1{};
LeituraTemperatura t2{};
Resultado res{0, true, false, false, false};

uint32_t marcaLeituraMs = 0;     // grade do ciclo de leitura: 1000, 2000, 3000...
uint32_t marcaMs = 0;            // grade do registro
uint32_t ultimoRegistroMs = 0;   // quando saiu a última linha, para o dt

void loop() {
  atenderSerial(cfg);

  if (ntpSincronizou) {
    ntpSincronizou = false;
    if (rtcPresente) {
      rtc.adjust(DateTime((uint32_t)time(nullptr)));
      horaOk = true;
      Serial.printf("[%lu ms] RTC ajustado por NTP: %sZ\n", millis(), rtc.now().timestamp().c_str());
    } else {
      Serial.printf("[%lu ms] NTP ok, mas RTC ausente: hora nao gravada\n", millis());
    }
  }

  // Foto dos valores: o Ticker pode trocá-los no meio deste trecho.
  bool fdvAgora = fdv;
  bool pressAgora = press;
  bool btnAgora = btn;

  if (fdvAgora != fdvAnterior) {
    Serial.printf("[%lu ms] FDV: %d -> %d\n", millis(), fdvAnterior, fdvAgora);
    fdvAnterior = fdvAgora;
  }
  if (pressAgora != pressAnterior) {
    Serial.printf("[%lu ms] PRESS: %d -> %d\n", millis(), pressAnterior, pressAgora);
    pressAnterior = pressAgora;
  }

  uint32_t agora = millis();
  if (agora - marcaLeituraMs >= PERIODO_LEITURA_MS) {
    marcaLeituraMs += PERIODO_LEITURA_MS;
    t1 = sensorT1.ler();
    t2 = sensorT2.ler();

    Entradas e;
    e.pressaoOk = !pressAgora;   // contato fechado (pressão OK) puxa o pino para 0
    e.t1 = t1.celsius;
    e.t1Falha = t1.falha;
    e.t2 = t2.celsius;
    e.t2Falha = t2.falha;
    e.fdv = (fdvAgora == (cfg.fdvNivelDesvio != 0)) ? Fdv::DESVIO : Fdv::DIRETO;   // D1
    e.botaoLimpeza = !btnAgora;  // apertado liga ao GND

    alarme.definirParametros(cfg.alarme);
    res = alarme.avaliar(e, agora);
    digitalWrite(PINO_K1, res.torreVermelha ? LOW : HIGH);   // segurança positiva: verde = energizado

    if (res.condicoes != anterior.condicoes || res.torreVermelha != anterior.torreVermelha ||
        res.modoLimpeza != anterior.modoLimpeza) {
      Serial.printf("[%lu ms] condicoes: 0x%02X -> 0x%02X | torre %s | limpeza %d\n", agora,
                    anterior.condicoes, res.condicoes, res.torreVermelha ? "VERMELHA" : "VERDE",
                    res.modoLimpeza);
      anterior = res;
    }
  }

  if (agora - marcaMs >= cfg.periodoRegistroMs) {
    marcaMs += cfg.periodoRegistroMs;  // grade fixa: atraso não se acumula
    uint32_t dt = agora - ultimoRegistroMs;
    ultimoRegistroMs = agora;
    // FW-07 / seção 7: leitura inválida sai sem valor (campo vazio), nunca um número falso.
    char t1Txt[8] = "";
    char t2Txt[8] = "";
    if (res.t1Valida) snprintf(t1Txt, sizeof(t1Txt), "%.1f", t1.celsius);
    if (res.t2Valida) snprintf(t2Txt, sizeof(t2Txt), "%.1f", t2.celsius);
    // FW-08: hora UTC em ISO 8601; vazia se o RTC não tem hora confiável (nunca uma hora falsa).
    // shortcut: rtc.now() não acusa erro de I2C se o módulo cair depois do boot; checar no autoteste (FW-14).
    char hora[24] = "";
    if (horaOk) snprintf(hora, sizeof(hora), "%sZ", rtc.now().timestamp().c_str());
    Serial.printf("%s [%lu ms] dt=%lu | T1=%s t1_valida=%d falha=0x%02X | T2=%s t2_valida=%d falha=0x%02X | cond=0x%02X\n",
                  hora, agora, dt, t1Txt, res.t1Valida, t1.falha, t2Txt, res.t2Valida, t2.falha, res.condicoes);
  }
}
