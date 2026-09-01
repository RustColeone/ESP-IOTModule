const devicesNode = document.getElementById('devices');
const noticeNode = document.getElementById('notice');
const onlineCountNode = document.getElementById('onlineCount');

function text(value) {
  return value === undefined || value === null ? '—' : String(value);
}

function statusRows(device) {
  const status = device.status_data || {};
  if (device.type === 'power-switch') {
    return [
      ['Power jack', status.powerJack ? 'On' : 'Off'],
      ['USB output', status.usbOutput ? 'On' : 'Off'],
      ['PD input', status.vbus === undefined ? '—' : `${status.vbus} V`],
    ];
  }
  if (device.type === 'fan-controller' && Array.isArray(status.fans)) {
    return status.fans.map((fan, index) => [
      `Fan ${index + 1}`,
      `${text(fan.speed)}% · ${text(fan.rpm)} RPM`,
    ]);
  }
  return [['Status', device.online ? 'Responding' : 'Unavailable']];
}

function makeElement(tag, className, content) {
  const node = document.createElement(tag);
  if (className) node.className = className;
  if (content !== undefined) node.textContent = content;
  return node;
}

function renderDevice(device) {
  const card = makeElement('article', `device-card ${device.online ? '' : 'offline'}`);
  const cardHeader = makeElement('div', 'card-header');
  const heading = makeElement('div');
  heading.append(makeElement('h2', '', device.display_name));
  heading.append(makeElement('p', 'model', device.model || device.type));
  cardHeader.append(heading);
  cardHeader.append(makeElement('span', `state ${device.online ? 'online' : ''}`, device.online ? 'Online' : 'Offline'));
  card.append(cardHeader);

  const facts = makeElement('dl', 'facts');
  for (const [label, value] of statusRows(device)) {
    facts.append(makeElement('dt', '', label));
    facts.append(makeElement('dd', '', value));
  }
  card.append(facts);

  const network = makeElement('div', 'network');
  network.append(makeElement('span', '', `${device.address}:${device.port}`));
  network.append(makeElement('code', '', device.id));
  card.append(network);

  const actions = makeElement('div', 'actions');
  const rename = makeElement('button', 'secondary', 'Rename');
  rename.addEventListener('click', () => renameDevice(device));
  const open = makeElement('a', 'primary', 'Open controls');
  open.href = device.url;
  open.target = '_blank';
  open.rel = 'noreferrer';
  if (!device.online) {
    open.classList.add('disabled');
    open.removeAttribute('href');
  }
  actions.append(rename, open);
  card.append(actions);
  return card;
}

async function renameDevice(device) {
  const alias = window.prompt('Friendly name (leave empty to restore the default):', device.alias || '');
  if (alias === null) return;
  await fetch(`/api/devices/${encodeURIComponent(device.id)}/alias`, {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({alias}),
  });
  await refresh();
}

async function refresh() {
  try {
    const response = await fetch('/api/devices', {cache: 'no-store'});
    const {devices} = await response.json();
    devicesNode.replaceChildren(...devices.map(renderDevice));
    const online = devices.filter(device => device.online).length;
    onlineCountNode.textContent = online;
    noticeNode.hidden = devices.length > 0;
    if (!devices.length) noticeNode.textContent = 'Searching for ESP-IOT devices on this network…';
  } catch (error) {
    noticeNode.hidden = false;
    noticeNode.textContent = 'The manager service is not responding.';
  }
}

refresh();
setInterval(refresh, 3000);
