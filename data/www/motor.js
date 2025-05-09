document.addEventListener('DOMContentLoaded', function() {
    const connectRangeInput = (rangeId, inputId) => {
        const rangeInput = document.getElementById(rangeId);
        const numberInput = document.getElementById(inputId);
        rangeInput.addEventListener('input', function() {
            numberInput.value = this.value;
        });
        numberInput.addEventListener('input', function() {
            rangeInput.value = this.value;
        });
    }
    connectRangeInput('speed-range', 'speed-number');
    connectRangeInput('angle-range', 'angle-number');
    connectRangeInput('position-range', 'position-number');

    const motorindex = document.querySelectorAll('input[name="motorIndex"]');
    const sendSpeedButton = document.querySelector("#speed-section #send-speed");
    const stopSpeedButton = document.querySelector("#speed-section #stop-speed");
    const enableButton = document.querySelector("#enable");
    const disableButton = document.querySelector("#disable");
    const sendStepsButton = document.querySelector("#steps-section button");
    const sendPositionButton = document.querySelector("#position-section button");
    const stopAllButton = document.querySelector("#stop-all");

    const selectedMotor = () => {
        let motor = 0;
        for (var i = 0, length = motorindex.length; i < length; i++) {
            if (motorindex[i].checked) {
                return i;
            }
          }
        return 0;
    }

    const setFeedback = (sectionName, message, color) => {
        const element = document.querySelector(`${sectionName} .feedback`);
        element.style.color = color;
        element.textContent = message;

    }

    const sendPostCommand = (command, sectionName, extraBody) => {
        const body = 
            {
                command,
                index: selectedMotor(),
                ...extraBody
            };
        console.log('Sending command ' + command, selectedMotor());
        fetch(`/api/motors`, {
            method: 'POST', 
            body: JSON.stringify(body),
            headers: {
                'Content-Type': 'application/json'
            }
        })
            .then(res => {
                if (res.ok) {
                    setFeedback(sectionName, 'OK', 'green');
                } else {
                    console.error(res);
                    setFeedback(sectionName, 'API error, see console', 'red');
                }
            })
            .catch(e => {
                console.error(e);
                setFeedback(sectionName, 'API error, see console', 'red');
            });
    };
    sendSpeedButton.addEventListener("click", () => sendPostCommand("runAtSpeed", "#speed-section", 
        {
            parameters: {
                speed: Number(document.getElementById('speed-number').value)
            }
        }));
    stopSpeedButton.addEventListener("click", () => sendPostCommand("runAtSpeed", "#speed-section", 
        {
            parameters: {
                speed: 0
            }
        }));
    sendStepsButton.addEventListener("click", () => sendPostCommand("runSteps", "#steps-section", 
        {
            parameters: {
                angle: Number(document.getElementById('angle-number').value),
                rpm: Number(document.getElementById('rpm-number').value),
            }
        }));
    sendPositionButton.addEventListener("click", () => sendPostCommand("moveToPosition", "#position-section", 
        {
            parameters: {
                position: Number(document.getElementById('position-number').value),
            }
        }));
    
    
    // on ebale button click, send POST to /api/motors with command: "enable"
    enableButton.addEventListener("click", () => sendPostCommand("enable", "#enable-disable-section"));
    // on disable button click, send POST to /api/motors with command: "disable"
    disableButton.addEventListener("click", () => sendPostCommand("disable", "#enable-disable-section"));
    // on stop all button click, send POST to /api/motors with command: "stopAll"
    stopAllButton.addEventListener("click", () => sendPostCommand("stopAll", "#emergency-section"));
});
