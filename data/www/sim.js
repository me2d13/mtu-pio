/* Simulator values the unit is receiving, refreshed with every state message. */

function setText(id, value) {
  const element = document.getElementById(id);
  if (element) element.textContent = value;
}

function percent(value) {
  if (value === undefined || value === null) return '-';
  return (value * 100).toFixed(1) + '%';
}

MTU.on('state', (data) => {
  const sim = data.simData;
  if (!sim) return;

  setText('sim-throttle1', sim.throttle1);
  setText('sim-throttle1-pct', percent(sim.throttle1));
  setText('sim-throttle2', sim.throttle2);
  setText('sim-throttle2-pct', percent(sim.throttle2));
  setText('sim-speed-brake', sim.speedBrake);
  setText('sim-speed-brake-pct', percent(sim.speedBrake));
  setText('sim-trim', sim.trim < 0 ? 'no data' : sim.trim);
  setText('sim-parking-brake', sim.parkingBrake ? 'on' : 'off');
  setText('sim-auto-throttle', sim.autoThrottle ? 'on' : 'off');
  setText('sim-age', sim.lastUpdate ? sim.ageMs + ' ms ago' : 'never');
});
