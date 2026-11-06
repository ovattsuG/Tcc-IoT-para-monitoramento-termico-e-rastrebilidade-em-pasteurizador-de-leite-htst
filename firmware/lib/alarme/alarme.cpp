// Implementação das regras de alarme (FW-03). Interface em alarme.h.
#include "alarme.h"

Alarme::Alarme(const Parametros& p) : p_(p) {}

Resultado Alarme::avaliar(const Entradas& e, uint32_t agoraMs) {
  Resultado r{0, false};
  if (!e.pressaoOk) r.condicoes |= PRESSAO;

  // Histerese: entre T_MIN e T_MIN + HIST mantém o estado; T1 inválido também mantém.
  if (e.t1Valida) {
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
  if (contandoDireto_ && agoraMs - diretoDesdeMs_ > p_.tFdvMs) r.condicoes |= SUBTEMP_SEM_DESVIO;

  // Só as condições críticas da seção 5 deixam a torre vermelha.
  r.torreVermelha = (r.condicoes & (PRESSAO | SUBTEMP_SEM_DESVIO)) != 0;
  return r;
}
