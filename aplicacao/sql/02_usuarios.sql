-- Usuários da aplicação (SW-04): nodered só grava, grafana só lê. Rodar como postgres, conectado ao banco htst.
-- As senhas NÃO ficam aqui: definir à parte com ALTER ROLE ... PASSWORD, sem salvar.

CREATE ROLE nodered LOGIN;
CREATE ROLE grafana LOGIN;

-- Por padrão qualquer usuário conecta em qualquer banco; aqui só os dois.
REVOKE CONNECT ON DATABASE htst FROM PUBLIC;
GRANT CONNECT ON DATABASE htst TO nodered, grafana;
GRANT USAGE ON SCHEMA public TO nodered, grafana;

-- Sem UPDATE e sem DELETE para ninguém: o dado original nunca muda.
GRANT INSERT ON leituras, rejeitadas TO nodered;
GRANT SELECT ON leituras, rejeitadas TO grafana;
