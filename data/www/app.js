/* Shared WebSocket connection + helpers, loaded on every page. */
const MTU = (() => {
  const handlers = {};
  let ws = null;
  let retryTimer = null;
  let lastState = null;

  function setConnection(state, text) {
    const dot = document.getElementById('conn-dot');
    const label = document.getElementById('conn-text');
    if (dot) dot.className = 'dot ' + state;
    if (label) label.textContent = text;
  }

  function connect() {
    const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
    ws = new WebSocket(protocol + '//' + location.host + '/ws');

    ws.onopen = () => setConnection('on', 'connected');

    ws.onmessage = (event) => {
      let message;
      try {
        message = JSON.parse(event.data);
      } catch (e) {
        console.error('bad websocket message', e);
        return;
      }
      const type = message.type || 'state';
      const data = message.data === undefined ? message : message.data;
      if (type === 'state') lastState = data;
      if (handlers[type]) handlers[type](data);
    };

    ws.onerror = () => setConnection('off', 'connection error');
    ws.onclose = () => {
      setConnection('off', 'disconnected, retrying');
      retryTimer = setTimeout(connect, 2000);
    };
  }

  async function post(url, body) {
    const response = await fetch(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: body === undefined ? '' : JSON.stringify(body),
    });
    const text = await response.text();
    if (!response.ok) throw new Error(text || response.statusText);
    return text;
  }

  document.addEventListener('DOMContentLoaded', () => {
    setConnection('off', 'connecting');
    connect();
  });

  return { on: (type, fn) => { handlers[type] = fn; }, state: () => lastState, post };
})();
