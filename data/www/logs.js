/* Live log entries: the page renders a snapshot, the websocket then appends new ones. */
const list = document.getElementById('logs');
const MAX_LINES = 200;

function addEntry(entry) {
  const pos = entry.indexOf(' - ');
  const li = document.createElement('li');
  const stamp = document.createElement('span');
  const msg = document.createElement('span');

  stamp.className = 'stamp';
  msg.className = 'msg';
  if (pos === -1) {
    msg.textContent = entry;
  } else {
    stamp.textContent = entry.slice(0, pos);
    msg.textContent = entry.slice(pos + 3);
  }
  if (msg.textContent.includes('WARN')) li.className = 'warn';

  li.append(stamp, msg);
  list.prepend(li);

  while (list.children.length > MAX_LINES) list.removeChild(list.lastChild);
}

MTU.on('logs', (entries) => {
  list.innerHTML = '';
  entries.forEach(addEntry);
});

MTU.on('logDelta', (entries) => entries.forEach(addEntry));
