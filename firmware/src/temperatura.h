// FW-01: temperatura por PT100 + MAX31865, atrás de uma interface simples
// para o resto do firmware não depender do chip.
#pragma once
#include <stdint.h>
#include <Adafruit_MAX31865.h>

struct LeituraTemperatura {
  float celsius;
  uint8_t falha;  // 0 = ok; senão, código de falha da MAX31865
};

class SensorTemperatura {
public:
  virtual ~SensorTemperatura() = default;
  virtual bool iniciar() = 0;
  virtual LeituraTemperatura ler() = 0;
};

class SensorPT100 : public SensorTemperatura {
public:
  SensorPT100(uint8_t pinoCs, uint8_t fios);
  bool iniciar() override;
  LeituraTemperatura ler() override;

private:
  Adafruit_MAX31865 max_;
  uint8_t fios_;
};
