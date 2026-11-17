/* Load and save the persisted configuration as JSON. */
const area = document.getElementById('config');
const feedback = document.getElementById('feedback');

function say(message, ok) {
  feedback.textContent = message;
  feedback.className = 'feedback ' + (ok ? 'ok' : 'bad');
}

function load() {
  fetch('/api/state')
    .then((response) => response.json())
    .then((data) => {
      area.value = JSON.stringify(data.persisted, null, 2);
      say('configuration loaded', true);
    })
    .catch((error) => {
      console.error(error);
      say('could not load configuration', false);
    });
}

document.getElementById('save').addEventListener('click', () => {
  let parsed;
  try {
    parsed = JSON.parse(area.value);
  } catch (error) {
    console.error(error);
    say('invalid JSON', false);
    return;
  }
  MTU.post('/api/state', parsed)
    .then((text) => say(text || 'saved', true))
    .catch((error) => {
      console.error(error);
      say('save failed: ' + error.message, false);
    });
});

document.getElementById('reload').addEventListener('click', load);

document.getElementById('reset').addEventListener('click', () => {
  MTU.post('/api/state', { factoryReset: true })
    .then((text) => {
      say(text || 'factory reset done - save to persist', true);
      load();
    })
    .catch((error) => {
      console.error(error);
      say('reset failed: ' + error.message, false);
    });
});

load();
