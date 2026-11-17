/* Motor commands through /api/motors. */

function selectedMotor() {
  const checked = document.querySelector('input[name="motorIndex"]:checked');
  return checked ? Number(checked.value) : 0;
}

function showFeedback(section, message, ok) {
  const element = document.getElementById('feedback-' + section);
  if (!element) return;
  element.textContent = message;
  element.className = 'feedback ' + (ok ? 'ok' : 'bad');
}

function command(section, name, parameters) {
  const body = { command: name, index: selectedMotor() };
  if (parameters) body.parameters = parameters;

  MTU.post('/api/motors', body)
    .then(() => showFeedback(section, 'OK', true))
    .catch((error) => {
      console.error(error);
      showFeedback(section, 'error: ' + error.message, false);
    });
}

function linkRange(rangeId, numberId) {
  const range = document.getElementById(rangeId);
  const number = document.getElementById(numberId);
  range.addEventListener('input', () => { number.value = range.value; });
  number.addEventListener('input', () => { range.value = number.value; });
}

linkRange('speed-range', 'speed-number');
linkRange('angle-range', 'angle-number');
linkRange('position-range', 'position-number');

document.getElementById('send-speed').addEventListener('click', () =>
  command('speed', 'runAtSpeed', { speed: Number(document.getElementById('speed-number').value) }));
document.getElementById('stop-speed').addEventListener('click', () =>
  command('speed', 'runAtSpeed', { speed: 0 }));
document.getElementById('send-steps').addEventListener('click', () =>
  command('steps', 'runSteps', {
    angle: Number(document.getElementById('angle-number').value),
    rpm: Number(document.getElementById('rpm-number').value),
  }));
document.getElementById('send-position').addEventListener('click', () =>
  command('position', 'moveToPosition', { position: Number(document.getElementById('position-number').value) }));
document.getElementById('enable').addEventListener('click', () => command('enable', 'enable'));
document.getElementById('disable').addEventListener('click', () => command('enable', 'disable'));
document.getElementById('stop-all').addEventListener('click', () => command('emergency', 'stopAll'));
