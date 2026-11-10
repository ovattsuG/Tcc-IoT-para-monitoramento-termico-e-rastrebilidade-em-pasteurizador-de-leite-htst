// FW-09 / FW-12: grava as linhas CSV da seção 7 no MicroSD (HSPI), um arquivo por dia UTC.
// FW-10 / FW-11: publica o JSON ao vivo; sem broker, guarda em /PENDENTES.txt e reenvia em ordem quando ele volta.
// O cartão é tratado numa tarefa própria do FreeRTOS: o loop() só põe a linha numa fila e segue,
// então um cartão travado ou removido não atrasa as regras de alarme nem a torre (FW-04, FW-11).
#pragma once

// Chamar no setup(): monta o cartão (FW-14), cria a fila e a tarefa. true = cartão montado.
bool iniciarTarefaSd();

// Põe o registro na fila sem esperar. horaUtc "2026-11-07T..." → /20261107.csv; "" → /SEMHORA.csv.
// json "" = só CSV (o JSON não coube). false = fila cheia (cartão travado há vários segundos): descartado, vira FALHA_SD.
bool enfileirarLinhaSd(const char* horaUtc, const char* linha, const char* json);

// true = cartão ausente, erro de gravação ou fila cheia (condição FALHA_SD). Some na próxima gravação boa.
bool sdComFalha();
