# Especificação de Requisitos de Firmware — Edge-HTST

| Campo | Valor |
|---|---|
| Projeto | Arquitetura IoT para Pasteurizadores HTST |
| Firmware | Edge-HTST (nó de borda ESP32) |
| Identificador | FW-HTST-01 |
| Versão | 1.5 |
| Data | 02/11/2026 |
| Autor | Gustavo Correa A. Moreira |
| Status | Em revisão |

## Histórico de revisões

| Versão | Data | Alteração |
|---|---|---|
| 1.0–1.3 | 28–31/10/2026 | Especificação inicial; interfaces (2× PT100/MAX31865, RTC DS3231, MicroSD, indutivo M12, pressostato, relé); correção do MISO do MicroSD; FDV apenas monitorada. |
| 1.4 | 31/10/2026 | Histerese de 0,5 °C; modos Alerta e Falha Segura; watchdog; detecção de falha do PT100; RTC com NTP em UTC; falha do MicroSD. |
| 1.5 | 02/11/2026 | **Correção da regra de alarme:** temperatura baixa com a válvula desviando só é registrada e não aciona a torre. Um único relé (K1) comanda luz vermelha e sirene juntas; falhas que não põem o produto em risco não acionam a torre e seguem só para a aplicação. Falha Segura deixa de exigir religar o equipamento. Modo limpeza. Faixa válida de temperatura. Reenvio sem duplicar (`seq`). Detalhes de implementação (pinos, períodos internos, ferramentas de medição) saíram dos requisitos e foram para o Anexo A. |

---

## 1. Objetivo e escopo

O firmware roda em um nó ESP32 instalado junto ao pasteurizador. Ele **lê**, de forma independente do CLP, a temperatura de pasteurização e de saída, a posição da válvula de desvio (FDV) e a pressão de ar do acionamento. Também **avalia localmente** as regras de alarme, **aciona a torre de sinalização** e **registra e envia** os dados para a aplicação.

**Dentro do escopo**
- Leitura de 2 PT100 (elemento livre das sondas duplas) via MAX31865.
- Leitura do sensor indutivo da FDV e do pressostato da linha de ar.
- Regras de alarme e acionamento da torre (luz verde; luz vermelha com sirene).
- Botão de modo limpeza.
- Hora por RTC DS3231, ajustada por NTP.
- Registro em cartão MicroSD e envio por MQTT, com guarda e reenvio quando a rede cai.
- Watchdog e autoteste de inicialização.

**Fora do escopo**
- Comando da FDV ou de qualquer atuador do processo. **O firmware só lê.**
- Backend, banco de dados, dashboard, relatórios, login e notificação remota (especificação de software separada).
- Autenticação e criptografia (TLS) do MQTT: o broker opera na rede local do laticínio. Registrar como limitação no TCC.
- Placa de circuito impresso e mecânica de instalação.

**Referência de forma:** ISO/IEC/IEEE 29148:2018 (requisitos verificáveis, com critério de aceitação).

## 2. Contexto

- **13/04/2026:** uma falha no ar comprimido impediu a FDV de desviar. Leite abaixo de 72 °C seguiu para a produção, e o CLP não emitiu alerta.
- **02/04/2026:** o sensor de saída do CLP falhou e foi para o fundo de escala (~100 °C). O CLP não emitiu alarme, e a falha só apareceu na rastreabilidade, mais de duas semanas depois.
- **Rastreabilidade:** os registros do CLP são retirados por pendrive em intervalos de duas semanas a um mês.

O firmware existe para que essas três situações sejam percebidas em segundos.

## 3. Sinais lógicos

O mapa de pinos não faz parte dos requisitos: ver o Anexo A.

| Sinal | Origem | Significado |
|---|---|---|
| `T1` | PT100_1 + MAX31865 #1 | Temperatura de pasteurização (ponto de controle da FDV) [D6] |
| `T2` | PT100_2 + MAX31865 #2 | Temperatura de saída do produto para a queijaria |
| `FDV` | Sensor indutivo M12 | `DIRETO` ou `DESVIO` [D1: confirmar qual nível lógico é qual] |
| `PRESS` | Pressostato (contato seco) | Fechado = pressão OK; aberto = falta de ar |
| `BTN_LIMP` | Botão de pulso | Iniciar modo limpeza |
| `K1` | Relé SPDT → torre | **Energizado = luz verde (contato NA); desenergizado = luz vermelha e sirene (contato NF)** |

O `K1` segue **segurança positiva**: se o nó perder alimentação ou travar, o relé desenergiza, e a torre fica **vermelha com sirene**. [D4: a torre precisa ser alimentada pela fonte do painel, e não pela fonte do nó.] A sirene fica ligada em paralelo com a luz vermelha, no contato NF. [D5: conferir se a corrente da luz vermelha somada à da sirene cabe no contato do relé.]

## 4. Parâmetros configuráveis

Os valores podem ser alterados sem recompilar o firmware (arquivo de configuração no cartão ou NVS).

| Parâmetro | Valor | Observação |
|---|---|---|
| `T_MIN` | 72,0 °C | RIISPOA, art. 255, §1º, II |
| `HIST` | 0,5 °C | A condição de subtemperatura só termina com `T1 ≥ T_MIN + HIST` |
| `T_FDV` | **a medir em campo** (provisório: 5 s) | Tempo que a FDV leva para desviar depois da queda [D2] |
| `FAIXA_VALIDA` | 0 a 105 °C | Fora disso, a leitura é inválida. Base: no CSV de 16–17/10/2026 os máximos foram 92,5 °C (pasteurização) e 85,1 °C (saída) [D7] |
| `TEMPO_LIMPEZA` | 4 h (provisório) | Duração máxima do modo limpeza [D3] |
| `PERIODO_REGISTRO` | 1 s | Uma linha no SD e uma mensagem MQTT por segundo |

## 5. Regras de alarme

As condições são avaliadas a cada ciclo de leitura. **Todas as condições ativas são registradas e enviadas**; a tabela define só o que a torre mostra.

| Condição | Quando ocorre | Torre |
|---|---|---|
| `SUBTEMP_SEM_DESVIO` | Modo produção **e** `T1 < T_MIN` **e** `FDV = DIRETO` por mais de `T_FDV` | **Vermelho + sirene** |
| `PRESSAO` | `PRESS` aberto (filtrado contra ruído) | **Vermelho + sirene** |
| `FALHA_T1` | MAX31865 #1 acusa falha **ou** `T1` fora de `FAIXA_VALIDA`, em 2 leituras seguidas | **Vermelho + sirene** (sem `T1` não há como verificar a pasteurização) |
| `FALHA_T2` | Mesmo critério, para `T2` | Verde: só registro e envio (afeta a rastreabilidade, não o produto) |
| `WDT` | O nó reiniciou por watchdog | Verde depois do reinício: só registro e envio |
| `FALHA_SD` | Cartão ausente, cheio ou com erro de gravação | Verde: só registro e envio |
| `SUBTEMP` | `T1 < T_MIN` **e** `FDV = DESVIO` (desvio funcionando) | **Verde: só registro** |
| `OK` | Nenhuma das anteriores | Verde |

**Comportamento**
- **Retorno ao verde:** quando a condição deixa de existir, a torre volta ao verde sozinha. A ocorrência fica no registro com início e fim.
- **Sem silenciamento:** com um único relé, a sirene toca enquanto a condição existir. [D5: avaliar na semana de operação se isso é aceitável. Alternativa: um segundo relé só para a sirene, mais um botão de reconhecimento.]
- **Modo limpeza:** o `BTN_LIMP` suspende **apenas** o `SUBTEMP_SEM_DESVIO`, por no máximo `TEMPO_LIMPEZA`, e volta sozinho ao modo produção. `PRESSAO` e `FALHA_T1` continuam ativas. Início e fim do modo são registrados. [D3]

## 6. Requisitos

| ID | Requisito | Critério de aceitação |
|---|---|---|
| FW-01 | Ler `T1` e `T2` com resolução de 0,1 °C e período de no máximo 1 s | Em 1 h de bancada, nenhum intervalo entre registros maior que 1 s |
| FW-02 | Ler `FDV` e `PRESS` com filtro contra ruído de contato | Pulsos menores que 50 ms são ignorados; mudança estável é reconhecida em menos de 200 ms |
| FW-03 | Aplicar as regras da seção 5 | Os 7 cenários do Quadro 1 do TCC produzem o resultado esperado |
| FW-04 | Acionar a torre em até **2 s** depois que a condição de alarme se configura (descontado o `T_FDV`) | 30 repetições por cenário na bancada, medidas pelo microcontrolador de teste (TCC, seção 3.3) |
| FW-05 | Manter `K1` desenergizado do reset até o fim do autoteste, e em qualquer travamento ou perda de alimentação | Cortar a alimentação do nó deixa a torre vermelha e com sirene |
| FW-06 | Modo limpeza pelo `BTN_LIMP`, como na seção 5 | Em limpeza, o cenário 3 não aciona a torre, e o cenário 4 aciona; depois de `TEMPO_LIMPEZA`, o cenário 3 volta a acionar |
| FW-07 | Detectar falha de sensor pelo registrador de falhas do MAX31865 e pela `FAIXA_VALIDA` | Abrir ou pôr em curto o PT100 gera `FALHA_T1` ou `FALHA_T2` em até 2 leituras; a leitura é gravada como inválida, sem valor de temperatura |
| FW-08 | Marcar a hora com o RTC DS3231, ajustado por NTP sempre que houver rede, em UTC (ISO 8601) | Depois de partir sem rede, os registros têm hora correta |
| FW-09 | Gravar 1 linha por segundo no MicroSD no formato da seção 7 | Arquivo íntegro depois de 24 h; leituras inválidas marcadas |
| FW-10 | Publicar 1 mensagem MQTT por segundo (JSON, QoS 1), com `seq` crescente e persistente | O consumidor descarta mensagens repetidas pelo par `device_id` + `seq` |
| FW-11 | Sem rede ou sem broker, continuar gravando no SD e, quando a conexão voltar, enviar as pendências em ordem, sem atrasar as regras de alarme | Com 24 h sem rede: nenhuma linha perdida, nenhuma duplicada no banco e FW-04 atendido durante a queda |
| FW-12 | Com falha do SD, manter monitoramento, alarmes e MQTT e sinalizar `FALHA_SD` | Remover o cartão em operação não altera a torre e gera `FALHA_SD` |
| FW-13 | Reiniciar por watchdog se o laço principal travar por mais de 2 s; depois do reinício, registrar e enviar `WDT` | Um laço infinito induzido reinicia o nó, e o evento `WDT` aparece no registro |
| FW-14 | Na inicialização, testar os 2 MAX31865, o RTC e o SD antes de liberar a operação | Com um periférico desligado, o nó sobe e sinaliza a falha correspondente da seção 5 |

## 7. Formato do registro e da telemetria

**CSV no MicroSD** (separador `,`, decimal `.`, UTF-8 sem BOM, um arquivo por dia em UTC):

```
seq,timestamp_utc,t1_c,t2_c,t1_valida,t2_valida,fdv,pressao_ok,modo,alarmes
10231,2026-11-02T13:05:01Z,75.4,33.1,1,1,DIRETO,1,PRODUCAO,
10232,2026-11-02T13:05:02Z,71.6,33.0,1,1,DESVIO,1,PRODUCAO,SUBTEMP
10233,2026-11-02T13:05:03Z,,33.0,0,1,DESVIO,1,PRODUCAO,FALHA_T1|SUBTEMP
```

- `alarmes` lista **todas** as condições ativas, separadas por `|`. Vazio significa OK.
- Leitura inválida: o campo de temperatura fica vazio e `*_valida = 0`. Nunca se grava um valor falso.

**JSON no MQTT** (tópico `htst/<device_id>/leituras`): os mesmos campos, mais `device_id`.

O `seq` não pode depender do relógio, porque um ajuste de NTP para trás geraria horários repetidos. Ele é um contador gravado em memória não volátil.

## 8. Decisões em aberto

| ID | Decisão | Como resolver |
|---|---|---|
| D1 | Que nível do sensor indutivo corresponde a `DESVIO` | Testar na válvula |
| D2 | Valor de `T_FDV` | Medir o tempo de desvio na máquina; o próprio nó pode medir isso nos primeiros dias |
| D3 | Se o modo limpeza é necessário e qual a duração máxima | Confirmar com a produção em que posição fica a FDV durante a limpeza e a partida |
| D4 | Fonte de alimentação da torre | Confirmar com a manutenção |
| D5 | Corrente da luz vermelha mais a sirene no contato do `K1`; e se a sirene sem silenciamento é aceitável na operação | Ver os dados da torre; avaliar na semana de operação |
| D6 | Posição exata da PT100_1: seção de retenção ou antes do tubo de retenção | Foto ou desenho do pasteurizador |
| D7 | `FAIXA_VALIDA` definitiva | Ajustar com um mês de CSV |

## Anexo A — Projeto de hardware (referência, não é requisito)

Mapa de pinos mantido da v1.4. A linha **proposta** é nova.

| Periférico | Pinos ESP32 |
|---|---|
| MAX31865 #1 (T1) | VSPI: MOSI 23, MISO 19, SCK 18, CS 5 |
| MAX31865 #2 (T2) | VSPI: MOSI 23, MISO 19, SCK 18, CS 17 |
| MicroSD | HSPI: SCK 14, MISO 32, MOSI 13, CS 15 |
| RTC DS3231 | I2C: SDA 21, SCL 22 |
| Sensor indutivo M12 NPN | GPIO 27 (pull-up 3,3 V e proteção) |
| Pressostato | GPIO 26 (pull-up interno) |
| `K1` relé da torre | GPIO 4 |
| `BTN_LIMP` | GPIO 34, só entrada, **pull-up externo** (**proposto**) |
| Depuração | UART0: TX 1, RX 3, 115200 8N1 |

Plataforma: ESP32 dual-core, 4 MB de flash, Wi-Fi 2,4 GHz, entradas a 3,3 V. Sinais de 24 V precisam de condicionamento.
