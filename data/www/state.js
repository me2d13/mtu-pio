/* Live hardware readings rendered from the "state" websocket messages. */

function setText(id, value) {
  const element = document.getElementById(id);
  if (element) element.textContent = value;
}

function setSwitch(id, on) {
  const element = document.getElementById(id);
  if (element) element.classList.toggle('on', on);
}

MTU.on('state', (data) => {
  if (data.axisValues && data.axisCalibratedValues) {
    for (let i = 0; i < data.axisValues.length; i++) {
      setText('axis-raw-' + i, data.axisValues[i]);
      setText('axis-cal-' + i, data.axisCalibratedValues[i]);
    }
  }

  if (data.axisReadFailures) {
    for (let i = 0; i < data.axisReadFailures.length; i++) {
      setText('axis-fail-' + i, data.axisReadFailures[i]);
    }
  }

  if (data.buttonsRawValue !== undefined) {
    for (let i = 0; i < 16; i++) {
      setSwitch('btn-' + i, (data.buttonsRawValue & (1 << i)) !== 0);
    }
  }

  setText('trim-wheel', data.trimWheelPosition);
  setText('rotary-value', data.rotaryEncoderValue);
  setText('rotary-presses', data.rotaryButtonPressedCount);

  const dot12v = document.getElementById('dot-12v');
  const val12v = document.getElementById('val-12v');
  if (dot12v && val12v) {
    dot12v.className = 'dot ' + (data['12VPresent'] ? 'on' : 'warn');
    val12v.textContent = data['12VPresent'] ? 'present' : 'absent';
  }

  setText('val-i2c-failures', data.i2cChannelSwitchFailures);
});
