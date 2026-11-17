/* Simulator values received over UDP, plus the mock form that pushes values back in. */

const MOCK_FIELDS = [
  { id: 'mock-throttle1', key: 'throttle1' },
  { id: 'mock-throttle2', key: 'throttle2' },
  { id: 'mock-speed-brake', key: 'speedBrake' },
  { id: 'mock-trim', key: 'trim' },
  { id: 'mock-parking-brake', key: 'parkingBrake', boolean: true },
  { id: 'mock-auto-throttle', key: 'autoThrottle', boolean: true },
];

function setText(id, value) {
  const element = document.getElementById(id);
  if (element) element.textContent = value;
}

function buildPayload() {
  const payload = {};
  MOCK_FIELDS.forEach((field) => {
    const element = document.getElementById(field.id);
    if (!element) return;
    const raw = element.value;
    if (raw === '') return;
    payload[field.key] = field.boolean ? raw === 'true' : Number(raw);
  });
  return payload;
}

function updatePayloadPreview() {
  setText('mock-payload', JSON.stringify(buildPayload()));
}

MOCK_FIELDS.forEach((field) => {
  const element = document.getElementById(field.id);
  if (!element) return;
  element.addEventListener('input', updatePayloadPreview);
  element.addEventListener('change', updatePayloadPreview);
});

function showFeedback(message, ok) {
  const element = document.getElementById('feedback-mock');
  if (!element) return;
  element.textContent = message;
  element.className = 'feedback ' + (ok ? 'ok' : 'bad');
}

document.getElementById('mock-send').addEventListener('click', () => {
  const payload = buildPayload();
  if (Object.keys(payload).length === 0) {
    showFeedback('Nothing to send - fill in at least one field.', false);
    return;
  }

  MTU.post('/api/sim', payload)
    .then((response) => showFeedback('Applied: ' + response, true))
    .catch((error) => {
      console.error(error);
      showFeedback('error: ' + error.message, false);
    });
});

document.getElementById('mock-fill').addEventListener('click', () => {
  const sim = MTU.state() && MTU.state().simData;
  if (!sim) {
    showFeedback('No simulator data received yet.', false);
    return;
  }
  document.getElementById('mock-throttle1').value = Number(sim.throttle1).toFixed(2);
  document.getElementById('mock-throttle2').value = Number(sim.throttle2).toFixed(2);
  document.getElementById('mock-speed-brake').value = Number(sim.speedBrake).toFixed(2);
  document.getElementById('mock-trim').value = sim.trim < 0 ? '' : Number(sim.trim).toFixed(1);
  document.getElementById('mock-parking-brake').value = sim.parkingBrake ? 'true' : 'false';
  document.getElementById('mock-auto-throttle').value = sim.autoThrottle ? 'true' : 'false';
  updatePayloadPreview();
});

document.getElementById('mock-clear').addEventListener('click', () => {
  MOCK_FIELDS.forEach((field) => {
    const element = document.getElementById(field.id);
    if (element) element.value = '';
  });
  updatePayloadPreview();
});

MTU.on('state', (data) => {
  const sim = data.simData;
  if (!sim) return;

  setText('sim-throttle1', Number(sim.throttle1).toFixed(2));
  setText('sim-throttle2', Number(sim.throttle2).toFixed(2));
  setText('sim-speed-brake', Number(sim.speedBrake).toFixed(2));
  setText('sim-trim', sim.trim < 0 ? 'no data' : Number(sim.trim).toFixed(2));
  setText('sim-parking-brake', sim.parkingBrake ? 'on' : 'off');
  setText('sim-auto-throttle', sim.autoThrottle ? 'on' : 'off');
  setText('sim-auto-throttle-value', sim.autoThrottle ? 'on' : 'off');

  // Whole seconds - the point is whether data is still arriving, not the exact age.
  const ageMs = sim.ageMs || 0;
  const seconds = Math.round(ageMs / 1000);
  setText('sim-age', sim.lastUpdate ? (seconds < 1 ? 'just now' : seconds + ' s ago') : 'never received');
  const liveDot = document.getElementById('sim-live-dot');
  if (liveDot) {
    if (!sim.lastUpdate) liveDot.className = 'dot off';
    else liveDot.className = 'dot ' + (ageMs <= 2000 ? 'on' : 'warn');
  }

  // Show the live values as placeholders so it is obvious what a field would overwrite.
  const placeholders = {
    'mock-throttle1': Number(sim.throttle1).toFixed(2),
    'mock-throttle2': Number(sim.throttle2).toFixed(2),
    'mock-speed-brake': Number(sim.speedBrake).toFixed(2),
    'mock-trim': sim.trim < 0 ? 'no data' : Number(sim.trim).toFixed(1),
    'mock-parking-brake': sim.parkingBrake ? 'on' : 'off',
    'mock-auto-throttle': sim.autoThrottle ? 'on' : 'off',
  };
  Object.keys(placeholders).forEach((id) => {
    const element = document.getElementById(id);
    if (element) element.placeholder = placeholders[id];
  });
});
