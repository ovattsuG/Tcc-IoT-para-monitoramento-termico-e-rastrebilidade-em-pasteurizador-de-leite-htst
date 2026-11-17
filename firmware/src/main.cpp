// FW-02: lê a válvula de desvio (FDV), o pressostato (PRESS) e o BTN_LIMP a cada 10 ms, com filtro de ruído.
// FW-01: lê T1 e T2 (PT100 + MAX31865) a cada 1 s. FW-03/04/06: avalia as regras da seção 5 e comanda a torre.
// FW-05: relé K1 desenergizado do reset até uma avaliação verde, a partir da 2ª.
// Seção 4: parâmetros carregados da NVS e ajustáveis pela Serial (get / set NOME valor).
// FW-07/08/09/10/12: registro da seção 7 (seq, hora UTC, inválida sem valor) gravado no MicroSD.
// FW-13: watchdog de 2 s no loop(); depois de um reinício por watchdog, WDT no 1º registro.
// FW-14: autoteste no setup() (2 MAX31865, RTC, SD); o K1 só é liberado a partir da 2ª avaliação.
#include <Arduino.h>
#include <Ticker.h>
#include "config.h"
#include "debounce.h"
#include "temperatura.h"
#include "configuracao.h"
#include "parametros_nvs.h"
#include "comandos_serial.h"
#include "alarme.h"
#include "sequencia.h"
#include "registro.h"
#include "cartao_sd.h"
#include "rede_mqtt.h"
#include <RTClib.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <esp_task_wdt.h>
#include "segredos.h"

Ticker tickerEntradas;
Configuracao cfg = configuracaoPadrao();
SensorPT100 sensorT1(PINO_CS_T1, FIOS_PT100);
SensorPT100 sensorT2(PINO_CS_T2, FIOS_PT100);
Alarme alarme(cfg.alarme);
Sequencia sequencia(0, BLOCO_SEQ);   // trocada no setup() pelo valor da NVS
RTC_DS3231 rtc;
bool rtcPresente = false;
bool horaOk = false;   // FW-08: false = RTC não responde ou perdeu a hora; o registro sai sem hora
bool relogioEspOk = false;   // NTP acertou o relógio interno do ESP: reserva quando o RTC falha
bool wdtPendente = false;   // FW-13: o boot veio de um watchdog e o 1º registro ainda não saiu

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
  // FW-13: TASK_WDT = laço travado; INT_WDT = interrupções travadas; WDT = outros watchdogs do chip.
  esp_reset_reason_t motivo = esp_reset_reason();
  wdtPendente = motivo == ESP_RST_TASK_WDT || motivo == ESP_RST_INT_WDT || motivo == ESP_RST_WDT;
  Serial.printf("boot: motivo do reinicio = %d%s\n", (int)motivo, wdtPendente ? " (watchdog)" : "");
  carregarConfiguracao(cfg);
  sequencia = Sequencia(carregarReservaSeq(), BLOCO_SEQ);
  imprimirConfiguracao(cfg);

  pinMode(PINO_FDV, INPUT_PULLUP);
  pinMode(PINO_PRESS, INPUT_PULLUP);
  pinMode(PINO_BTN_LIMP, INPUT);   // GPIO 34 não tem pull-up interno: o de 10 kΩ é externo
  tickerEntradas.attach_ms(PERIODO_AMOSTRA_MS, lerEntradas);

  // FW-14: autoteste. Quem não responde continua sendo testado em ler() e vira FALHA_T1/FALHA_T2 (seção 5).
  bool t1Ok = sensorT1.iniciar();
  if (!t1Ok) Serial.println("T1: MAX31865 nao responde");
  bool t2Ok = sensorT2.iniciar();
  if (!t2Ok) Serial.println("T2: MAX31865 nao responde");
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
  iniciarMqtt();                       // FW-10: conecta em segundo plano
  bool sdOk = iniciarTarefaSd();   // FW-09/12/14: monta aqui; depois o cartão é tratado em outra tarefa
  Serial.printf("autoteste: T1 %s | T2 %s | RTC %s | SD %s\n", t1Ok ? "ok" : "FALHA", t2Ok ? "ok" : "FALHA",
                !rtcPresente ? "FALHA" : (horaOk ? "ok" : "sem hora"), sdOk ? "ok" : "FALHA");

  // FW-13: o núcleo já cria o watchdog de tarefas com 5 s, e chamar init de novo não trocou o tempo na bancada
  // (disparou em 6 s e 14,6 s). Por isso desmonta e cria de novo com 2 s; para desmontar, tira o IDLE0 antes.
  // Por último: o setup() pode levar mais de 2 s (Wi-Fi, SD) e não está no caminho da torre.
  disableCore0WDT();
  bool wdtOk = esp_task_wdt_deinit() == ESP_OK && esp_task_wdt_init(TEMPO_WATCHDOG_S, true) == ESP_OK &&
               esp_task_wdt_add(nullptr) == ESP_OK;
  enableCore0WDT();
  if (!wdtOk) Serial.println("WDT: erro ao ligar o watchdog do loop()");
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
Entradas ent{};
Resultado res{0, true, false, false, false};

bool primeiraAvaliacao = true;
uint32_t marcaLeituraMs = 0;     // grade do ciclo de leitura: 1000, 2000, 3000...
uint32_t marcaMs = 0;            // grade do registro
uint32_t ultimoRegistroMs = 0;   // quando saiu a última linha, para o dt

void loop() {
  esp_task_wdt_reset();   // FW-13: se o loop() parar de passar aqui por 2 s, o chip reinicia
  atenderSerial(cfg);

  if (ntpSincronizou) {
    ntpSincronizou = false;
    relogioEspOk = true;
    if (rtcPresente) {
      rtc.adjust(DateTime((uint32_t)time(nullptr)));
      horaOk = true;
      Serial.printf("[%lu ms] RTC ajustado por NTP: %sZ\n", millis(), rtc.now().timestamp().c_str());
    } else {
      Serial.printf("[%lu ms] NTP ok, mas RTC ausente: usando o relogio do ESP\n", millis());
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
    // Offset antes de tudo: faixa válida, alarme, SD e MQTT veem a mesma temperatura corrigida.
    t1.celsius += cfg.offsetT1C;
    t2.celsius += cfg.offsetT2C;

    Entradas e{};   // {} zera tudo: campo esquecido vira false/0, nunca lixo
    e.sdFalha = sdComFalha();
    e.reinicioWdt = wdtPendente;
    e.pressaoOk = !pressAgora;   // contato fechado (pressão OK) puxa o pino para 0
    e.t1 = t1.celsius;
    e.t1Falha = t1.falha;
    e.t2 = t2.celsius;
    e.t2Falha = t2.falha;
    e.fdv = (fdvAgora == (cfg.fdvNivelDesvio != 0)) ? Fdv::DESVIO : Fdv::DIRETO;   // D1
    e.botaoLimpeza = !btnAgora;  // apertado liga ao GND

    alarme.definirParametros(cfg.alarme);
    res = alarme.avaliar(e, agora);
    // FW-05/FW-14: a 1ª avaliação não libera o K1; FALHA_T1 precisa de 2 leituras e só aparece na 2ª.
    if (primeiraAvaliacao) {
      primeiraAvaliacao = false;
      res.torreVermelha = true;
    }
    ent = e;
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
    // FW-10: a reserva vai para a NVS ANTES de usar o número; se falhar, tenta de novo no próximo registro.
    if (sequencia.precisaGravar()) {
      if (salvarReservaSeq(sequencia.reservaParaGravar())) sequencia.reservaGravada();
      else Serial.println("seq: erro ao gravar a reserva na NVS; o numero pode repetir apos reinicio");
    }

    // FW-08: hora UTC em ISO 8601; vazia se o RTC não tem hora confiável (nunca uma hora falsa).
    // rtc.now() não acusa erro de I2C: com o módulo solto devolve lixo ("2000-00-01T43:101:00", visto na bancada).
    // isValid() recusa data impossível; aí o registro sai sem hora, nunca com hora falsa.
    // shortcut: um lixo que forme uma data possível passaria; se acontecer, comparar com o registro anterior.
    char hora[24] = "";
    if (horaOk) {
      DateTime agoraRtc = rtc.now();
      if (agoraRtc.isValid()) snprintf(hora, sizeof(hora), "%sZ", agoraRtc.timestamp().c_str());
      else Serial.println("RTC: leitura invalida (I2C); registro sem hora");
    }
    // Reserva: RTC ausente ou com leitura inválida, mas o NTP já acertou o relógio do ESP (decisão de 15/11/2026).
    // Some num reinício sem rede; por isso o RTC continua sendo a fonte principal.
    if (hora[0] == '\0' && relogioEspOk) {
      time_t t = time(nullptr);
      strftime(hora, sizeof(hora), "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));
    }

    // Seção 7: leitura inválida sai sem valor (FW-07); a validade vem do alarme, a mesma regra da torre.
    Registro reg{sequencia.proximo(), hora, t1.celsius, res.t1Valida, t2.celsius, res.t2Valida,
                 ent.fdv, ent.pressaoOk, res.modoLimpeza, res.condicoes};
    char linha[160];
    if (formatarCsv(reg, linha, sizeof(linha)) == 0) {
      Serial.println("registro: linha nao coube; nada gravado");
    } else {
      // FW-10 / FW-11: a tarefa do SD grava o CSV e decide entre publicar ao vivo e guardar como pendência.
      char json[320];
      if (formatarJson(reg, DEVICE_ID, json, sizeof(json)) == 0) {
        Serial.println("registro: JSON nao coube; so o CSV");
        json[0] = '\0';
      }
      if (!enfileirarLinhaSd(hora, linha, json)) Serial.println("SD: fila cheia; linha descartada");   // FW-09 / FW-12
      else wdtPendente = false;   // FW-13: WDT só até o 1º registro entregue à tarefa do SD
      Serial.printf("[%lu ms] dt=%lu falha1=0x%02X falha2=0x%02X | %s\n", agora, dt, t1.falha, t2.falha, linha);
    }
  }
}
