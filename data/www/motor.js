document.addEventListener('DOMContentLoaded', function() {
    const rangeInput = document.getElementById('speed');
    const numberInput = document.getElementById('speed-number');
    const sendSpeed = document.querySelector("#speed-section button");
    const motorindex = document.querySelectorAll('input[name="motorIndex"]');

    rangeInput.addEventListener('input', function() {
        numberInput.value = this.value;
    });

    const pickMotor = () => {
        let motor = 0;
        for (var i = 0, length = motorindex.length; i < length; i++) {
            if (motorindex[i].checked) {
                return i;
            }
          }
        return 0;
    }

    const setFeedback = (sectionName, message, color) => {
        const element = document.querySelector(`#${sectionName} #feedback`);
        element.style.color = color;
        element.textContent = message;

    }

    sendSpeed.addEventListener("click", () => {
        const body = [
            {
                command: "runAtSpeed",
                index: pickMotor(),
                parameters: {
                    speed: numberInput.value
                }
            }
        ];
        console.log('Sending speed', pickMotor(), numberInput.value);
        fetch(`/api/motors`, {method: 'POST', body: JSON.stringify(body)})
            .then(res => {
                if (res.ok) {
                    setFeedback('speed-section', 'OK', 'green');
                } else {
                    console.error(res);
                    setFeedback('speed-section', 'API error, see console', 'red');
                }
            })
            .catch(e => {
                console.error(e);
                setFeedback('speed-section', 'API error, see console', 'red');
            });
    });
});
