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
  return responde;
}

// Bloqueia ~75 ms (conversão da MAX31865). Não decide alarme: só informa.
LeituraTemperatura SensorPT100::ler() {
  LeituraTemperatura leitura;
  leitura.celsius = max_.temperature(RESISTENCIA_NOMINAL_PT100, RESISTENCIA_REF);
  leitura.falha = max_.readFault();
  if (leitura.falha != 0) {
    max_.clearFault();  // limpa para a próxima leitura detectar de novo
  }
  return leitura;
}
