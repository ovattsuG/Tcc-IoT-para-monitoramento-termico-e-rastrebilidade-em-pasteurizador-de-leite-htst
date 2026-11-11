# Matriz de verificação da aplicação — requisito → teste

Especificação: `spec/SRS_Aplicacao-HTST_v1.0.md`. Plano: `docs/plano-aplicacao.md`.

Status possíveis: Pendente · Implementado – aguardando teste · Aprovado · Reprovado.

| Req. | Critério de aceitação (resumo) | Procedimento | Status | Data | Resultado e evidência |
|---|---|---|---|---|---|
| SW-01 | Broker só na rede local | Nó publica; dispositivo de fora não conecta | Pendente | | |
| SW-02 | Linhas no banco = `seq` publicados em 1 h | Consulta SQL depois de 1 h (e no teste de 24 h) | Implementado – aguardando teste | 10/11/2026 | Parcial: fluxo `aplicacao/nodered/flows.json` (MQTT QoS 1 → `validar` → INSERT). Primeiro teste com o ESP: 721 linhas, `seq` máx. 14990, `rejeitadas` vazia (inclui o reenvio das pendências do SD). Falta a contagem de 1 h sem buracos |
| SW-03 | Reenviar 100 já gravadas não gera linha nova | `mosquitto_pub -l` com `aplicacao/testes/sw03_100_mensagens.txt` (100 mensagens, `device_id = teste`, `seq` 1 a 100), publicado 3 vezes | Aprovado | 10/11/2026 | `SELECT count(*) FROM leituras WHERE device_id = 'teste'` → 100 depois da 1ª publicação e 100 depois das republicações. Antes, no banco: o mesmo par inserido duas vezes pelo `nodered` não dá erro (`teste_sw04_permissoes.sql`) |
| SW-04 | `nodered` só INSERT, `grafana` só SELECT; UPDATE de qualquer um falha | `aplicacao/sql/teste_sw04_permissoes.sql` | Aprovado | 10/11/2026 | 10 de 10 `PASSOU`: `nodered` grava em `leituras` e `rejeitadas` e é negado em UPDATE, DELETE e SELECT; `grafana` lê e é negado em UPDATE, DELETE e INSERT. 1ª execução reprovou o INSERT com `ON CONFLICT (device_id, seq)` (exige SELECT nas colunas): trocado por `ON CONFLICT DO NOTHING` sem alvo |
| SW-05 | Mensagem malformada vai para `rejeitadas` | `mosquitto_pub -l` com `aplicacao/testes/sw05_invalidas.txt` (4 inválidas), publicado 2 vezes | Aprovado | 10/11/2026 | `SELECT motivo, conteudo FROM rejeitadas` → 8 linhas (4 × 2), cada uma com a mensagem bruta e o motivo: `JSON malformado: Unexpected end of JSON input`, `campo faltando: seq`, `valor inválido em fdv: "ABERTO"`, `t1_c incoerente com t1_valida`. Nenhuma foi para `leituras` |
| SW-06 | Dashboard ao vivo, ≤ 5 s | Inspeção | Pendente | | |
| SW-07 | Alarme visual em ≤ 10 s | Cronometrar do evento no nó ao painel | Pendente | | |
| SW-08 | Nó parado: vermelho em ≤ 70 s | Desligar o nó | Pendente | | |
| SW-09 | Inválidas como lacuna; alarmes como faixas | Cenários 5 e 6 | Pendente | | |
| SW-10 | Relatório do período, CSV e PDF | Gerar o relatório de um dia | Pendente | | |
| SW-11 | Sem login não abre | Abrir sem login | Pendente | | |
| SW-12 | Anotação "alarme visto" com o nome do usuário | Criar anotação | Pendente | | |
| SW-13 | Backup diário restaurável em outra máquina | Restaurar e conferir | Pendente | | |
| SW-14 | Grava em UTC, mostra em Brasília | Evento de 13:00 UTC aparece às 10:00 | Pendente | | |
