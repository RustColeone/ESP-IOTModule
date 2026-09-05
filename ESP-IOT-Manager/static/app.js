const devicesNode = document.getElementById('devices');
const noticeNode = document.getElementById('notice');
const onlineCountNode = document.getElementById('onlineCount');
const detailPanel = document.getElementById('detailPanel');
const detailFrame = document.getElementById('detailFrame');
const detailTitle = document.getElementById('detailTitle');
const openDirect = document.getElementById('openDirect');
const closeDetail = document.getElementById('closeDetail');
let editingControl = false;

function text(value) {
  return value === undefined || value === null ? '—' : String(value);
}

function statusRows(device) {
  const status = device.status_data || {};
  if (device.type === 'power-switch') {
    return [
      ['PD input', status.vbus === undefined ? '—' : `${status.vbus} V`],
      ['PD request', status.pdVoltage === undefined ? '—' : `${status.pdVoltage} V`],
    ];
  }
  if (device.type === 'fan-controller' && Array.isArray(status.fans)) {
    return status.fans.map((fan, index) => [
      `Fan ${index + 1} RPM`,
      `${text(fan.rpm)} RPM`,
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

function valueAtPath(source, path) {
  return String(path || '').split('.').reduce((value, part) => {
    if (value === undefined || value === null) return undefined;
    return value[part];
  }, source);
}

function renderQuickControls(device) {
  const controls = Array.isArray(device.controls) ? device.controls : [];
  if (!controls.length) return null;

  const section = makeElement('section', 'quick-controls');
  section.append(makeElement('p', 'control-heading', 'Quick controls'));
  const list = makeElement('div', 'control-list');

  for (const control of controls) {
    const currentValue = valueAtPath(device.status_data, control.statePath);
    if (control.type === 'toggle') {
      const row = makeElement('div', 'control-row toggle-row');
      row.append(makeElement('span', 'control-label', control.label || control.id));
      const button = makeElement('button', `toggle-button ${currentValue ? 'on' : ''}`, currentValue ? 'On' : 'Off');
      button.type = 'button';
      button.disabled = !device.online;
      button.setAttribute('aria-pressed', currentValue ? 'true' : 'false');
      button.addEventListener('click', () => applyControl(device, control, !Boolean(currentValue), button));
      row.append(button);
      list.append(row);
      continue;
    }

    if (control.type === 'range') {
      const row = makeElement('label', 'control-row range-row');
      const labelLine = makeElement('span', 'range-label');
      labelLine.append(makeElement('span', 'control-label', control.label || control.id));
      const output = makeElement('output', '', `${text(currentValue)}${control.unit || ''}`);
      labelLine.append(output);
      const input = makeElement('input', 'range-control');
      input.type = 'range';
      input.min = control.min ?? 0;
      input.max = control.max ?? 100;
      input.step = control.step ?? 1;
      input.value = currentValue ?? input.min;
      input.disabled = !device.online;
      input.addEventListener('pointerdown', () => { editingControl = true; });
      input.addEventListener('keydown', () => { editingControl = true; });
      input.addEventListener('input', () => { output.textContent = `${input.value}${control.unit || ''}`; });
      input.addEventListener('change', async () => {
        editingControl = false;
        input.blur();
        await applyControl(device, control, Number(input.value), input);
      });
      input.addEventListener('pointercancel', () => { editingControl = false; });
      input.addEventListener('blur', () => { editingControl = false; });
      row.append(labelLine, input);
      list.append(row);
    }
  }
  section.append(list);
  return section;
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

  const quickControls = renderQuickControls(device);
  if (quickControls) card.append(quickControls);

  const network = makeElement('div', 'network');
  network.append(makeElement('span', '', `${device.address}:${device.port}`));
  network.append(makeElement('code', '', device.id));
  card.append(network);

  const actions = makeElement('div', 'actions');
  const rename = makeElement('button', 'secondary', 'Rename');
  rename.addEventListener('click', () => renameDevice(device));
  const open = makeElement('button', 'primary open-controls', 'Open controls');
  open.type = 'button';
  open.addEventListener('click', () => showDetail(device));
  if (!device.online) {
    open.classList.add('disabled');
    open.disabled = true;
  }
  actions.append(rename, open);
  card.append(actions);
  return card;
}

async function applyControl(device, control, value, input) {
  input.disabled = true;
  try {
    const response = await fetch(
      `/api/devices/${encodeURIComponent(device.id)}/controls/${encodeURIComponent(control.id)}`,
      {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({value}),
      },
    );
    const contentType = response.headers.get('Content-Type') || '';
    const result = contentType.includes('application/json') ? await response.json() : null;
    if (!response.ok) {
      const hint = response.status === 404
        ? ' Restart the ESP-IOT Manager so its updated backend route is loaded.'
        : '';
      throw new Error(result?.error || `Control request failed (${response.status}).${hint}`);
    }
    if (!result) throw new Error('The manager returned an unexpected non-JSON response.');
    if (result.deviceStatus) device.status_data = result.deviceStatus;
    await refresh();
  } catch (error) {
    window.alert(error.message || 'The device did not accept that control change.');
    await refresh();
  }
}

function showDetail(device) {
  detailTitle.textContent = device.display_name;
  detailFrame.title = `${device.display_name} controls`;
  detailFrame.src = device.url;
  openDirect.href = device.url;
  detailPanel.hidden = false;
  detailPanel.scrollIntoView({behavior: 'smooth', block: 'start'});
}

function hideDetail() {
  detailPanel.hidden = true;
  detailFrame.src = 'about:blank';
  openDirect.removeAttribute('href');
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
    if (!editingControl) devicesNode.replaceChildren(...devices.map(renderDevice));
    const online = devices.filter(device => device.online).length;
    onlineCountNode.textContent = online;
    noticeNode.hidden = devices.length > 0;
    if (!devices.length) noticeNode.textContent = 'Searching for ESP-IOT devices on this network…';
  } catch (error) {
    noticeNode.hidden = false;
    noticeNode.textContent = 'The manager service is not responding.';
  }
}

closeDetail.addEventListener('click', hideDetail);
refresh();
setInterval(refresh, 3000);
