/* Hardware calls and quick checks. */

function show(section, message, ok) {
  const element = document.getElementById('feedback-' + section);
  if (!element) return;
  element.textContent = message;
  element.className = 'feedback ' + (ok ? 'ok' : 'bad');
}

document.getElementById('scan-i2c').addEventListener('click', () => {
  const bus = document.getElementById('i2c-bus').value;
  MTU.post('/api/i2c-scan?index=' + bus)
    .then((text) => show('i2c', text, true))
    .catch((error) => {
      console.error(error);
      show('i2c', 'scan failed: ' + error.message, false);
    });
});

document.getElementById('debug-call').addEventListener('click', () => {
  const index = document.getElementById('debug-index').value;
  MTU.post('/api/debug?index=' + index)
    .then((text) => show('debug', text, true))
    .catch((error) => {
      console.error(error);
      show('debug', 'debug call failed: ' + error.message, false);
    });
});

document.getElementById('heap-check').addEventListener('click', () => {
  fetch('/heap')
    .then((response) => response.text())
    .then((text) => show('heap', Number(text) + ' bytes free', true))
    .catch((error) => {
      console.error(error);
      show('heap', 'request failed: ' + error.message, false);
    });
});
