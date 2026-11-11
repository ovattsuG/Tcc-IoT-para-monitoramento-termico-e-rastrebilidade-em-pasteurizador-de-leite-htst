-- Tabelas da aplicação (SRS seção 3; SW-02, SW-03, SW-05). Rodar conectado ao banco htst.

CREATE TABLE leituras (
  device_id     text         NOT NULL,
  seq           bigint       NOT NULL,
  timestamp_utc timestamptz,                -- nula se o nó estava sem hora (decisão 16 do plano)
  t1_c          numeric(5,1),
  t2_c          numeric(5,1),
  t1_valida     boolean      NOT NULL,
  t2_valida     boolean      NOT NULL,
  fdv           text         NOT NULL CHECK (fdv IN ('DIRETO', 'DESVIO')),
  pressao_ok    boolean      NOT NULL,
  modo          text         NOT NULL CHECK (modo IN ('PRODUCAO', 'LIMPEZA')),
  alarmes       text         NOT NULL DEFAULT '',
  recebido_em   timestamptz  NOT NULL DEFAULT now(),
  PRIMARY KEY (device_id, seq),             -- SW-03: o repetido bate aqui e o ON CONFLICT descarta
  -- Nunca gravar valor falso: temperatura só existe se a leitura é válida.
  CHECK (t1_valida = (t1_c IS NOT NULL)),
  CHECK (t2_valida = (t2_c IS NOT NULL))
);

-- O Grafana sempre filtra por período.
CREATE INDEX leituras_timestamp ON leituras (timestamp_utc);

CREATE TABLE rejeitadas (
  id          bigint GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  conteudo    text         NOT NULL,      -- mensagem bruta, como chegou
  motivo      text         NOT NULL,
  recebido_em timestamptz  NOT NULL DEFAULT now()
);
