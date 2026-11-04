# Registro de decisões de projeto

## Decisões tomadas

| Data | Decisão | Alternativas consideradas | Motivo |
|---|---|---|---|
| 02/11/2026 | A regra de alarme roda no próprio nó; a torre não depende de rede nem de servidor | Alarme decidido no servidor | O alerta precisa funcionar com o Wi-Fi fora (computação de borda, TCC 2.3) |
| 02/11/2026 | Um único relé (`K1`, SPDT): energizado = verde; desenergizado = vermelho + sirene | Dois relés (luz e sirene separadas) com botão de reconhecimento | Mais simples; segurança positiva mantida. Custo: a sirene não pode ser silenciada (avaliar na operação, D5) |
| 02/11/2026 | Temperatura baixa com a válvula desviando só é registrada (`SUBTEMP`) | Alarmar qualquer temperatura < 72 °C | O desvio funcionando é o comportamento correto; alarmar geraria alarme falso em todo desvio |
| 02/11/2026 | Falha do T2, watchdog e falha do SD não acionam a torre; só registro e envio | Acionar a torre em qualquer falha | Com relé único, acionar a torre = tocar a sirene; essas falhas não põem o produto em risco |
| 02/11/2026 | Modo limpeza por botão, com expiração automática | Entrada do CLP (intrusivo); nenhum modo (alarme em toda limpeza) | A fábrica não tem registro de quando há limpeza; a expiração evita esquecer o modo ligado |
| 02/11/2026 | O cartão SD funciona como buffer, e as pendências são reenviadas por MQTT com `seq` persistente | Usar o horário como identificador | Um ajuste de NTP para trás repetiria horários |
| 02/11/2026 | Comunicação por MQTT (QoS 1) | HTTP com o Node-RED consultando o nó | Publicação e assinatura com broker local; reenvio natural das pendências |
| 02/11/2026 | Aplicação com Node-RED, PostgreSQL e Grafana OSS, sem backend nem frontend próprios | React com backend próprio | Prazo; o Grafana já tem gráficos, login e anotações. Backend e frontend próprios ficam como evolução |
| 02/11/2026 | Sem notificação remota: alarme físico (torre) e visual (dashboard) | Telegram ou e-mail | Escolha do autor; simplifica. Custo: o alarme só chega a quem está perto da torre ou com o dashboard aberto |
| 02/11/2026 | Node-RED só valida e grava; nenhuma regra de alarme na aplicação | Duplicar as regras no servidor | As regras existem em um só lugar (o nó), sem risco de divergência |
| 02/11/2026 | Contas individuais no Grafana, todas com o mesmo papel | Uma conta compartilhada | A anotação "alarme visto" precisa identificar quem viu |
| 02/11/2026 | Relatório em PDF pela impressão do navegador; CSV pelo painel | Grafana Enterprise | O PDF nativo é recurso pago |
| 02/11/2026 | Broker Mosquitto separado do Node-RED | Broker embutido no Node-RED (`node-red-contrib-aedes`) | Reiniciar o Node-RED (frequente no desenvolvimento) não derruba a recepção nem força o nó a reenviar |
| 03/11/2026 | VS Code + PlatformIO, framework Arduino | Arduino IDE; ESP-IDF | Testes no PC e versões registradas (Arduino IDE não tem); ESP-IDF é mais trabalhoso para o prazo |
| 03/11/2026 | Entradas amostradas por `Ticker` a cada 10 ms, filtro de 100 ms | Ler as entradas só no `loop()` | Cada leitura de PT100 bloqueia ~75 ms; no `loop()` a mudança poderia passar dos 200 ms do FW-02 |
| 03/11/2026 | Bancada com resistor fixo em 2 fios; `FIOS_PT100` configurável | Fixar 3 fios no código | O resistor de teste tem 2 terminais; a sonda real provavelmente é de 3 fios (confirmar na fábrica) |

## Decisões em aberto (especificação, seção 8)

| ID | Decisão | Status | Resolução |
|---|---|---|---|
| D1 | Nível do indutivo que corresponde a `DESVIO` | Aberta | |
| D2 | Valor de `T_FDV` | Aberta | |
| D3 | Modo limpeza necessário? Duração máxima | Aberta | |
| D4 | Fonte de alimentação da torre | Aberta | |
| D5 | Corrente no contato do `K1`; sirene sem silenciamento é aceitável? | Aberta | |
| D6 | Posição da PT100_1 | Aberta | |
| D7 | `FAIXA_VALIDA` definitiva | Aberta | |
