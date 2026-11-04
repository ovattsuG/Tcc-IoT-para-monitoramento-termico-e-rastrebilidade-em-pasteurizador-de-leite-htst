# Matriz de verificação — requisito → teste

Cenários de bancada (TCC, Quadro 1):
1. Operação normal
2. Desvio correto
3. Leite subpasteurizado
4. Perda de ar
5. Sensor aberto
6. Sensor em curto
7. Perda de rede

Status possíveis: Pendente · Implementado – aguardando bancada · Aprovado · Reprovado.

| Req. | Critério de aceitação (resumo) | Cenário / procedimento | Status | Data | Resultado e evidência | Commit |
|---|---|---|---|---|---|---|
| FW-01 | Intervalo entre registros ≤ 1 s em 1 h | 1 h de bancada; análise do log da Serial (`dt`) e, no bloco 4, do CSV | Pendente | | | |
| FW-02 | Pulso < 50 ms ignorado; mudança estável < 200 ms | Testes no PC (`test_debounce`, 6 casos) + pulsos pelo microcontrolador de teste no indutivo e no jumper | Pendente | | | |
| FW-03 | Os 7 cenários dão o resultado esperado | Cenários 1 a 7 | Pendente | | | |
| FW-04 | Torre em ≤ 2 s (descontado `T_FDV`) | Cenários 3, 4 e 5, com 30 repetições cada pelos relés de teste | Pendente | | | |
| FW-05 | Sem alimentação: vermelho + sirene | Cortar a alimentação do nó | Pendente | | | |
| FW-06 | Modo limpeza suspende só o cenário 3 e expira | Cenários 3 e 4 em limpeza; esperar `TEMPO_LIMPEZA` (reduzido para o teste) | Pendente | | | |
| FW-07 | Falha de sensor detectada em ≤ 2 leituras | Cenários 5 e 6 em T1 e em T2 | Pendente | | | |
| FW-08 | Hora correta depois de partir sem rede | Desligar, religar sem Wi-Fi, comparar com a hora de referência | Pendente | | | |
| FW-09 | CSV íntegro em 24 h, inválidas marcadas | 24 h de bancada; validar o arquivo | Pendente | | | |
| FW-10 | Consumidor descarta mensagens repetidas por `device_id` + `seq` | Reenviar mensagens; conferir no banco | Pendente | | | |
| FW-11 | 24 h sem rede: sem perda, sem duplicata, FW-04 mantido | Cenário 7 | Pendente | | | |
| FW-12 | Sem SD: torre inalterada e `FALHA_SD` | Remover o cartão em operação | Pendente | | | |
| FW-13 | Travamento → reinício e `WDT` registrado | Laço infinito induzido | Pendente | | | |
| FW-14 | Periférico ausente no boot → falha sinalizada | Ligar com cada periférico desconectado | Pendente | | | |
