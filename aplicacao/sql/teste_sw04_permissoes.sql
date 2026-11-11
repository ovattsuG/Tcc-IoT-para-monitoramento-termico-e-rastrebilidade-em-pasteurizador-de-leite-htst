-- Teste do SW-04 (critério: um UPDATE do nodered ou do grafana falha). Rodar como postgres no banco htst.
-- Rodar o arquivo inteiro (F5, sem selecionar nada) e ler a aba Messages: toda linha tem que dizer PASSOU.
DO $$
DECLARE
  t record;
BEGIN
  FOR t IN SELECT * FROM (VALUES
    -- ON CONFLICT sem alvo: com alvo "(device_id, seq)" o PostgreSQL exige SELECT nessas colunas (visto no teste de 10/11/2026).
    ('nodered', true,  'INSERT INTO leituras (device_id, seq, t1_valida, t2_valida, fdv, pressao_ok, modo) VALUES (''teste'', 1, false, false, ''DIRETO'', true, ''PRODUCAO'') ON CONFLICT DO NOTHING'),
    -- SW-03: o mesmo seq de novo não dá erro e não grava outra linha.
    ('nodered', true,  'INSERT INTO leituras (device_id, seq, t1_valida, t2_valida, fdv, pressao_ok, modo) VALUES (''teste'', 1, false, false, ''DIRETO'', true, ''PRODUCAO'') ON CONFLICT DO NOTHING'),
    ('nodered', true,  'INSERT INTO rejeitadas (conteudo, motivo) VALUES (''{'', ''teste'')'),
    ('nodered', false, 'UPDATE leituras SET alarmes = ''X'' WHERE device_id = ''teste'''),
    ('nodered', false, 'DELETE FROM leituras WHERE device_id = ''teste'''),
    ('nodered', false, 'SELECT count(*) FROM leituras'),
    ('grafana', true,  'SELECT count(*) FROM leituras'),
    ('grafana', false, 'UPDATE leituras SET alarmes = ''X'' WHERE device_id = ''teste'''),
    ('grafana', false, 'DELETE FROM leituras WHERE device_id = ''teste'''),
    ('grafana', false, 'INSERT INTO rejeitadas (conteudo, motivo) VALUES (''{'', ''teste'')')
  ) AS v(papel, deve_funcionar, comando)
  LOOP
    BEGIN
      EXECUTE format('SET LOCAL ROLE %I', t.papel);
      EXECUTE t.comando;
      EXECUTE 'RESET ROLE';
      RAISE NOTICE '% | % | %', CASE WHEN t.deve_funcionar THEN 'PASSOU (funcionou)' ELSE 'FALHOU: deveria ser negado' END, t.papel, t.comando;
    EXCEPTION WHEN insufficient_privilege THEN
      -- O erro desfaz o SET ROLE junto: o laço volta a rodar como postgres.
      RAISE NOTICE '% | % | %', CASE WHEN t.deve_funcionar THEN 'FALHOU: deveria funcionar' ELSE 'PASSOU (negado)' END, t.papel, t.comando;
    END;
  END LOOP;

  -- Limpeza, como postgres.
  DELETE FROM leituras WHERE device_id = 'teste';
  DELETE FROM rejeitadas WHERE motivo = 'teste';
END $$;
