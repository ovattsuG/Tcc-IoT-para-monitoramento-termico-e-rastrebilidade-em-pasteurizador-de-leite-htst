# Instruções para o agente de IA — Edge-HTST

Firmware do nó de borda ESP32 que monitora, de forma independente do CLP, um pasteurizador HTST de leite: temperatura de pasteurização e de saída (2× PT100 via MAX31865), posição da válvula de desvio (sensor indutivo) e pressão de ar (pressostato). O nó aciona uma torre de sinalização, grava em cartão SD e publica via MQTT. Este repositório é parte de um TCC de Engenharia de Software e segue desenvolvimento guiado por especificação (SDD).

## Protocolo de colaboração com o agente

Autor: Gustavo Correa A. Moreira (TCC de Engenharia de Software, entrega em 17/11/2026). O agente atua como **par de programação**: propõe, justifica e explica; o autor decide, executa, valida e responde pelo resultado. O critério de sucesso não é só o firmware funcionar, mas o autor dominar e saber defender cada decisão e cada linha de código.

- Responda em português do Brasil, sem preâmbulo, resposta primeiro.
- **Um passo por vez.** Explique o que será feito e por quê, em poucas linhas; o autor executa e confirma o resultado antes do próximo passo.
- **Não gere arquivos inteiros de uma vez** nem execute comandos sem pedido do autor. Mostre o trecho, explique cada parte não óbvia; o autor digita ou aprova.
- Antes de código: 2 ou 3 linhas com a decisão e a alternativa descartada. Diga o que quebraria em produção.
- Se o autor estiver errado, diga. Se faltar informação, pergunte. Não invente versão de biblioteca nem comportamento de API.
- Todo código é revisado e testado pelo autor antes do commit.

## Estado atual (04/11/2026)

- VS Code + PlatformIO instalados. ESP32 genérico **DevKit V1, 30 pinos** (perfil PlatformIO `esp32doit-devkit-v1`), USB **CH9102 na COM5**, driver ok.
- Placas MAX31865 **ainda sem pinos soldados**: os testes de temperatura esperam a solda. Começar pelo que não precisa delas (projeto, Serial, entradas com jumper, testes no PC).
- PT100 da sonda: provavelmente 3 fios (confirmar na fábrica). Bancada com resistor = 2 fios.
- Decisões: `docs/decisoes.md`. Plano: `docs/plano.md`. Tarefas do bloco atual: `docs/tarefas.md`.
- 04/11/2026: projeto PlatformIO criado em `firmware/` (espressif32 7.1.3, `default_envs` = placa); "olá" na Serial validou placa, cabo e COM5. Abrir sempre pelo `edge-htst.code-workspace`.
- 04/11/2026: `docs/tarefas.md` do bloco 1 criado. Tarefa 1 concluída: `lib/debounce/` (100 ms, tempo passado de fora), 6/6 testes em `pio test -e native`.
- 04/11/2026: tarefa 2 concluída: `config.h`, `main.cpp` com `Ticker` 10 ms + debounce em GPIO 26/27 (`INPUT_PULLUP`), mudanças na Serial; aprovado com jumper.
- **Próximo passo:** tarefa 3 (`SensorPT100` com MAX31865), **bloqueada até soldar os pinos das placas MAX31865**.

## Fonte da verdade

- **`spec/FRS_Edge-HTST_v1.5.md` é a especificação.** Todo código deve atender a um requisito `FW-xx` dela.
- **Não crie requisito, função ou comportamento que não esteja na especificação.** Se achar que falta algo, proponha a mudança e espere aprovação antes de implementar.
- **Não suponha.** Se uma informação não está na especificação, **pergunte**. As decisões abertas (D1 a D7, seção 8) usam o valor provisório da especificação, sempre como parâmetro configurável, nunca fixo no código.
- Nunca altere `spec/` sem que eu peça.

## Fluxo de trabalho (SDD)

A especificação está pronta: **não refaça a fase Specify.** Trabalhe nesta ordem e **pare ao fim de cada etapa** para minha revisão:

1. **Plan:** proponha a estrutura de módulos, o framework e as bibliotecas, com a justificativa de cada escolha e a alternativa descartada. Salve em `docs/plano.md`. Não escreva código nesta etapa.
2. **Tasks:** quebre o bloco atual em tarefas pequenas, cada uma ligada a um ou mais `FW-xx`. Salve em `docs/tarefas.md`.
3. **Implement:** uma tarefa por vez. No fim de cada uma:
   - diga o que eu devo testar na bancada e qual resultado esperar;
   - atualize a linha correspondente em `docs/verificacao.md` (status "Implementado – aguardando bancada");
   - sugira a mensagem de commit.

Blocos, nesta ordem:

| Bloco | Requisitos |
|---|---|
| 1 | FW-01, FW-02: leituras |
| 2 | FW-03, FW-04, FW-05, FW-06: regras de alarme, relé e modo limpeza |
| 3 | FW-07, FW-08: falha de sensor e hora |
| 4 | FW-09 a FW-12: SD, MQTT e guarda e reenvio |
| 5 | FW-13, FW-14: watchdog e autoteste |

## Estrutura de pastas

- O código do firmware fica em `firmware/` (o `platformio.ini` também). `spec/` e `docs/` ficam na raiz.
- `notas/` é pessoal e fica fora do Git.

## Registro a cada tarefa concluída

1. `docs/verificacao.md`: atualize a linha do FW-xx com status, data e evidência (o que foi testado e o resultado, ex.: trecho da Serial ou resultado do teste).
2. `docs/decisoes.md`: se eu tomei alguma decisão, registre data, decisão, alternativa descartada e motivo.
3. `notas/aprendizado.md`: acrescente, em linguagem simples, o que aprendi. Um título por conceito, 2 a 5 linhas: o que é, para que serve no projeto e onde fica.
4. `notas/codigo/<modulo>.md` (um arquivo por módulo: `debounce.md`, `main.md`, ...): para cada arquivo de código novo ou alterado, uma seção com o código explicado linha a linha (comparando com Java quando ajudar), um fluxograma Mermaid da lógica e uma simulação "no papel" com valores concretos.

## Fechamento de bloco (quando eu escrever "fechar bloco N")

1. Confira se `docs/verificacao.md` está completo para todos os FW do bloco.
2. Crie ou atualize `docs/fluxograma.md` com um diagrama Mermaid do funcionamento do firmware até agora, acrescentando o que o bloco trouxe.
3. Revise `notas/aprendizado.md` e complete o que faltou.
4. Mostre um resumo do que mudou e sugira a mensagem de commit.

## Regras de código

- **A lógica de alarme (seção 5 da especificação) fica em um módulo próprio, sem nenhuma dependência de hardware ou de Arduino.** Ela recebe as leituras e o estado, devolve as condições ativas e o estado da torre, e tem testes unitários que rodam no PC cobrindo todas as linhas da tabela da seção 5 e os cenários de bancada. Esse módulo é o centro do TCC.
- O hardware fica atrás de interfaces simples (ler temperatura, ler entrada, acionar relé), para que a lógica possa ser testada sem a placa.
- **Nada de espera bloqueante no caminho das regras de alarme.** Rede, SD e MQTT não podem atrasar a torre (FW-04, FW-11).
- **Trate os erros de verdade:** nenhum erro de periférico é ignorado em silêncio, e todo erro vira uma condição da seção 5 ou uma linha de log.
- Os parâmetros da seção 4 da especificação são configuráveis sem recompilar.
- Credenciais de Wi-Fi e broker nunca vão para o repositório (arquivo de configuração fora do Git, com um exemplo versionado).
- Comentários: no topo de cada arquivo, 1 a 3 linhas com o que ele faz e qual `FW-xx` atende; no corpo, só o **porquê** do que não é óbvio. A explicação didática fica em `notas/codigo/`, não no código.
- **O firmware nunca comanda a válvula de desvio nem qualquer atuador do processo.** O único atuador é o relé `K1` da torre.

## Commits

Um commit por tarefa, com a mensagem começando pelo requisito, por exemplo: `FW-02: filtro de ruído nas entradas discretas`.

## Registro de decisões

Quando eu tomar uma decisão de projeto durante o trabalho, registre-a em `docs/decisoes.md` com data, decisão, alternativas consideradas e motivo.
