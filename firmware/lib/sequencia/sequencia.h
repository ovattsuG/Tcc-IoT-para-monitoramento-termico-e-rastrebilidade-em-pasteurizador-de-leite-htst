// FW-10: "seq" crescente e persistente, que nunca repete mesmo com reinícios.
// Reserva números em blocos: a NVS guarda o fim da reserva, gravado ANTES de usar os números. Lógica pura (PC).
#pragma once
#include <stdint.h>

class Sequencia {
public:
  // reservaGravada = valor lido da NVS (0 se nada gravado); bloco = quantos números por gravação.
  Sequencia(uint32_t reservaGravada, uint32_t bloco);

  // Se devolver true, gravar reservaParaGravar() na NVS ANTES de chamar proximo().
  bool precisaGravar() const { return atual_ >= reserva_; }
  uint32_t reservaParaGravar() const { return reserva_ + bloco_; }
  void reservaGravada() { reserva_ += bloco_; }

  uint32_t proximo() { return atual_++; }

private:
  uint32_t atual_;
  uint32_t reserva_;   // primeiro número que ainda NÃO está garantido pela NVS
  uint32_t bloco_;
};
