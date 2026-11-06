// Implementação das regras de alarme (FW-03). Interface em alarme.h.
#include "alarme.h"

// Para em 2: com um contador que só soma, depois de 255 ele voltaria a 0 e a falha sumiria sozinha.
static void contarInvalidas(uint8_t& seguidas, bool valida) {
  if (valida) seguidas = 0;
  else if (seguidas < 2) seguidas++;
}

Alarme::Alarme(const Parametros& p) : p_(p) {}

// Seção 5 / FW-07: inválida se o MAX31865 acusou falha ou se está fora da FAIXA_VALIDA.
bool Alarme::leituraValida(float celsius, uint8_t falha) const {
  return falha == 0 && celsius >= p_.faixaMinC && celsius <= p_.faixaMaxC;
}

Resultado Alarme::avaliar(const Entradas& e, uint32_t agoraMs) {
  Resultado r{0, false, false, false, false};

  // Modo limpeza (FW-06): entra no aperto; apertos durante a limpeza são ignorados (decisão de 06/11/2026).
  if (e.botaoLimpeza && !botaoAnterior_ && !limpeza_) {
    limpeza_ = true;
    limpezaDesdeMs_ = agoraMs;
  }
  botaoAnterior_ = e.botaoLimpeza;
  if (limpeza_ && agoraMs - limpezaDesdeMs_ >= p_.tempoLimpezaMs) limpeza_ = false;
  r.modoLimpeza = limpeza_;

  if (!e.pressaoOk) r.condicoes |= PRESSAO;

  r.t1Valida = leituraValida(e.t1, e.t1Falha);
  r.t2Valida = leituraValida(e.t2, e.t2Falha);
  contarInvalidas(invalidasT1_, r.t1Valida);
  contarInvalidas(invalidasT2_, r.t2Valida);
  if (invalidasT1_ >= 2) r.condicoes |= FALHA_T1;
  if (invalidasT2_ >= 2) r.condicoes |= FALHA_T2;

  // Histerese: entre T_MIN e T_MIN + HIST mantém o estado; T1 inválido também mantém.
  if (r.t1Valida) {
    if (e.t1 < p_.tMin) subtemp_ = true;
    else if (e.t1 >= p_.tMin + p_.hist) subtemp_ = false;
  }

  // Cronômetro do T_FDV: conta só enquanto há subtemperatura com a FDV em DIRETO, sem interrupção.
  bool diretoEmSubtemp = subtemp_ && e.fdv == Fdv::DIRETO;
  if (!diretoEmSubtemp) {
    contandoDireto_ = false;
  } else if (!contandoDireto_) {
    contandoDireto_ = true;
    diretoDesdeMs_ = agoraMs;
  }

  if (subtemp_ && e.fdv == Fdv::DESVIO) r.condicoes |= SUBTEMP;
  // "Por mais de T_FDV" (seção 5): exatamente T_FDV ainda não dispara.
  // A limpeza só esconde a condição; o cronômetro segue, e ao sair da limpeza o alarme vem na hora.
  if (!limpeza_ && contandoDireto_ && agoraMs - diretoDesdeMs_ > p_.tFdvMs) r.condicoes |= SUBTEMP_SEM_DESVIO;

  // Só as condições críticas da seção 5 deixam a torre vermelha (FALHA_T2 não: afeta só a rastreabilidade).
  r.torreVermelha = (r.condicoes & (PRESSAO | SUBTEMP_SEM_DESVIO | FALHA_T1)) != 0;
  return r;
}
