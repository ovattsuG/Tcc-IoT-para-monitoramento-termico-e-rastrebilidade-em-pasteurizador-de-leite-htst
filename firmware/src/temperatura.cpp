// PT100 + MAX31865 (FW-01) com teste de eco no início e enquanto o chip não responder (FW-14). Interface em temperatura.h.
#include "temperatura.h"
#include "config.h"

SensorPT100::SensorPT100(uint8_t pinoCs, uint8_t fios)
    : max_(pinoCs), fios_(fios) {}

bool SensorPT100::iniciar() {
  max31865_numwires_t modo = MAX31865_2WIRE;
  if (fios_ == 3) modo = MAX31865_3WIRE;
  if (fios_ == 4) modo = MAX31865_4WIRE;
  max_.begin(modo);

  // Teste de eco: prova que o chip responde no SPI.
  max_.setThresholds(0x1234, 0x5678);
  bool responde = max_.getLowerThreshold() == 0x1234 &&
                  max_.getUpperThreshold() == 0x5678;
  // Limites abertos: senão o chip acusaria falha fora de 0x1234..0x5678.
  max_.setThresholds(0x0000, 0xFFFF);
  responde_ = responde;
  return responde;
}

// Bloqueia ~75 ms (conversão da MAX31865). Não decide alarme: só informa.
LeituraTemperatura SensorPT100::ler() {
  // FW-14: sem eco, um fio solto pode ler um número dentro da faixa; refaz o begin() + eco até o chip responder.
  if (!responde_ && !iniciar()) return {0.0f, FALHA_SEM_RESPOSTA};
  LeituraTemperatura leitura;
  leitura.celsius = max_.temperature(RESISTENCIA_NOMINAL_PT100, RESISTENCIA_REF);
  leitura.falha = max_.readFault();
  if (leitura.falha != 0) {
    max_.clearFault();  // limpa para a próxima leitura detectar de novo
  }
  return leitura;
}
