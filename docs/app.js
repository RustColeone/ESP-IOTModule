const STORAGE_KEY = 'esp-iot-web.devices.v1';
const POLL_INTERVAL_MS = 5000;
const REQUEST_TIMEOUT_MS = 2500;

const connectForm = document.getElementById('connectForm');
const addressInput = document.getElementById('deviceAddress');
const connectMessage = document.getElementById('connectMessage');
const deviceGrid = document.getElementById('deviceGrid');
const emptyState = document.getElementById('emptyState');
const onlineCount = document.getElementById('onlineCount');
const refreshButton = document.getElementById('refreshButton');
const deviceTemplate = document.getElementById('deviceTemplate');

let devices = loadDevices();
const liveState = new Map();
let pollInProgress = false;

function loadDevices() {
  try {
    const stored = JSON.parse(localStorage.getItem(STORAGE_KEY) || '[]');
    return Array.isArray(stored) ? stored : [];
  } catch {
    return [];
  }
}

function saveDevices() {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(devices));
}

function normalizeAddress(value) {
  let candidate = value.trim();
  if (!candidate) throw new Error('Enter a device hostname or IP address.');
  if (!/^https?:\/\//i.test(candidate)) candidate = `http://${candidate}`;

  let url;
  try {
    url = new URL(candidate);
  } catch {
    throw new Error('That device address is not valid.');
  }
  if (!['http:', 'https:'].includes(url.protocol)) {
    throw new Error('The address must use HTTP or HTTPS.');
  }
  return url.origin;
}

async function fetchLocalJson(url) {
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), REQUEST_TIMEOUT_MS);
  try {
    const response = await fetch(url, {
      cache: 'no-store',
      mode: 'cors',
      targetAddressSpace: 'local',
      signal: controller.signal,
    });
    if (!response.ok) throw new Error(`Device returned HTTP ${response.status}.`);
    return await response.json();
  } catch (error) {
    if (error.name === 'AbortError') throw new Error('The device did not respond in time.');
    throw error;
  } finally {
    clearTimeout(timeout);
  }
}

function localApiUrl(device, path) {
  const result = new URL(path || '/', device.baseUrl);
  if (result.origin !== new URL(device.baseUrl).origin) {
    throw new Error('The device returned an unsafe API address.');
  }
  return result.href;
}

async function connectDevice(value) {
  const baseUrl = normalizeAddress(value);
  const metadata = await fetchLocalJson(`${baseUrl}/api/device`);
  if (metadata.protocol !== 'esp-iot/1' || !metadata.id) {
    throw new Error('This address did not identify itself as an ESP-IOT device.');
  }

  const existing = devices.find(device => device.id === metadata.id);
  const record = {
    id: metadata.id,
    baseUrl,
    alias: existing?.alias || '',
    name: metadata.name || metadata.model || metadata.id,
    type: metadata.type || 'unknown',
    model: metadata.model || 'ESP-IOT device',
    firmware: metadata.firmware || '',
    ui: metadata.ui || '/',
    status: metadata.status || '/api/status',
  };

  devices = devices.filter(device => device.id !== record.id);
  devices.push(record);
  saveDevices();
  await pollDevice(record);
}

function statusRows(device, status) {
  if (!status) return [['Status', 'Waiting for device']];
  if (device.type === 'power-switch') {
    return [
      ['Power jack', status.powerJack ? 'On' : 'Off'],
      ['USB output', status.usbOutput ? 'On' : 'Off'],
      ['PD input', status.vbus == null ? '—' : `${status.vbus} V`],
    ];
  }
  if (device.type === 'fan-controller' && Array.isArray(status.fans)) {
    return status.fans.map((fan, index) => [
      `Fan ${index + 1}`,
      `${fan.speed ?? '—'}% · ${fan.rpm ?? '—'} RPM`,
    ]);
  }
  return [['Status', 'Responding']];
}

function render() {
  deviceGrid.replaceChildren();
  emptyState.hidden = devices.length > 0;
  let online = 0;

  const sorted = [...devices].sort((a, b) => {
    const aOnline = liveState.get(a.id)?.online ? 0 : 1;
    const bOnline = liveState.get(b.id)?.online ? 0 : 1;
    return aOnline - bOnline || displayName(a).localeCompare(displayName(b));
  });

  for (const device of sorted) {
    const live = liveState.get(device.id) || {online: false};
    if (live.online) online += 1;
    const card = deviceTemplate.content.firstElementChild.cloneNode(true);
    card.classList.toggle('offline', !live.online);
    card.querySelector('.device-name').textContent = displayName(device);
    card.querySelector('.device-model').textContent = [device.model, device.firmware && `firmware ${device.firmware}`].filter(Boolean).join(' · ');

    const state = card.querySelector('.device-state');
    state.textContent = live.online ? 'Online' : 'Offline';
    state.classList.toggle('online', live.online);

    const statusList = card.querySelector('.device-status');
    for (const [label, value] of statusRows(device, live.status)) {
      const term = document.createElement('dt');
      const description = document.createElement('dd');
      term.textContent = label;
      description.textContent = value;
      statusList.append(term, description);
    }

    card.querySelector('.device-address').textContent = device.baseUrl.replace(/^https?:\/\//, '');
    card.querySelector('.device-id').textContent = device.id;

    const openButton = card.querySelector('.open-button');
    openButton.href = localApiUrl(device, device.ui);

    card.querySelector('.rename-button').addEventListener('click', () => renameDevice(device));
    card.querySelector('.remove-button').addEventListener('click', () => removeDevice(device));
    deviceGrid.append(card);
  }
  onlineCount.textContent = online;
}

function displayName(device) {
  return device.alias || device.name || device.id;
}

async function pollDevice(device) {
  try {
    const status = await fetchLocalJson(localApiUrl(device, device.status));
    liveState.set(device.id, {online: true, status, lastSeen: Date.now()});
  } catch (error) {
    const previous = liveState.get(device.id) || {};
    liveState.set(device.id, {...previous, online: false, error: error.message});
  }
}

async function pollAll() {
  if (pollInProgress) return;
  pollInProgress = true;
  refreshButton.disabled = true;
  try {
    await Promise.allSettled(devices.map(pollDevice));
    render();
  } finally {
    pollInProgress = false;
    refreshButton.disabled = false;
  }
}

function renameDevice(device) {
  const alias = window.prompt('Friendly name (leave empty to use the device default):', device.alias || '');
  if (alias === null) return;
  device.alias = alias.trim().slice(0, 80);
  saveDevices();
  render();
}

function removeDevice(device) {
  if (!window.confirm(`Remove ${displayName(device)} from this browser?`)) return;
  devices = devices.filter(item => item.id !== device.id);
  liveState.delete(device.id);
  saveDevices();
  render();
}

connectForm.addEventListener('submit', async event => {
  event.preventDefault();
  const button = connectForm.querySelector('button[type="submit"]');
  button.disabled = true;
  connectMessage.className = 'form-message';
  connectMessage.textContent = 'Waiting for the device and browser permission…';
  try {
    await connectDevice(addressInput.value);
    addressInput.value = '';
    connectMessage.classList.add('success');
    connectMessage.textContent = 'Device added to this browser.';
    render();
  } catch (error) {
    connectMessage.classList.add('error');
    connectMessage.textContent = error.message || 'Could not connect to that device.';
  } finally {
    button.disabled = false;
  }
});

refreshButton.addEventListener('click', pollAll);

const sharedDevice = new URLSearchParams(location.search).get('device');
if (sharedDevice) addressInput.value = sharedDevice;

render();
pollAll();
setInterval(pollAll, POLL_INTERVAL_MS);
