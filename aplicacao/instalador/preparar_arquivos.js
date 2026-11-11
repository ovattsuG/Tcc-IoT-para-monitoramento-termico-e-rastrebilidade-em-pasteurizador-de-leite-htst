// Prepara os arquivos de configuração do pendrive (chamado pelo preparar_pendrive.ps1).
// flows.json: usuário e senha do banco vêm do ambiente do serviço NodeRED, sem flows_cred.json.
// Dashboards: sem o uid do PC de desenvolvimento (o Grafana da fábrica cria o seu).
const fs = require('fs');
const path = require('path');
const [app, destino] = process.argv.slice(2);

const flows = JSON.parse(fs.readFileSync(path.join(app, 'nodered', 'flows.json'), 'utf8'));
const banco = flows.find(n => n.type === 'postgreSQLConfig');
if (!banco) throw new Error('flows.json sem o nó postgreSQLConfig');
// Nomes lidos por postgresql.js (node-red-contrib-postgresql 0.16.2) quando o tipo do campo é 'env'.
Object.assign(banco, {userFieldType: 'env', userEnv: 'HTST_DB_USUARIO', passwordFieldType: 'env', passwordEnv: 'HTST_DB_SENHA'});
fs.writeFileSync(path.join(destino, 'config', 'flows.json'), JSON.stringify(flows, null, 2));

for (const nome of ['ao-vivo.json', 'relatorio.json']) {
  const d = JSON.parse(fs.readFileSync(path.join(app, 'grafana', nome), 'utf8'));
  d.metadata = {name: d.metadata.name, namespace: 'default'};
  fs.writeFileSync(path.join(destino, 'grafana', nome), JSON.stringify(d, null, 2));
}
console.log('flows.json e dashboards prontos em', destino);
