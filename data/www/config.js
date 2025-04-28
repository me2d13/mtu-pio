document.addEventListener('DOMContentLoaded', function() {
    const saveButton = document.querySelector("#save");
    const resetButton = document.querySelector("#reset");
    const contentTextArea = document.querySelector("#config");
    const feedback = document.querySelector("#feedback");


    const setFeedback = (message, color) => {
        feedback.style.color = color;
        feedback.textContent = message;
    }

    // load state from /api/state
    fetch('/api/state')
        .then(res => res.json())
        .then(data => {
            contentTextArea.value = JSON.stringify(data.persisted, null, 2);
        })
        .catch(e => {
            console.error(e);
            setFeedback('API error, see console', 'red');
        });

    
    
    saveButton.addEventListener("click", () => {
        const content = contentTextArea.value;
        try {
            const parsedContent = JSON.parse(content);
            fetch('/api/state', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(parsedContent)
            })
            //.then(res => res.json())
            .then(res => {
                if (!res.ok) {
                    setFeedback('Network response was not ok ' + res.statusText, 'red');
                } else {
                    res.text().then(text => {
                        setFeedback(text, 'green');
                    });
                }
            })
            .catch(e => {
                console.error(e);
                setFeedback('API error, see console', 'red');
            });
        } catch (e) {
            console.error(e);
            setFeedback('Invalid JSON, see console', 'red');
        }
    });
    resetButton.addEventListener("click", () => {
            fetch('/api/state', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ factoryReset: true })
            })
            .then(res => {
                if (!res.ok) {
                    setFeedback('Network response was not ok ' + res.statusText, 'red');
                } else {
                    res.text().then(text => {
                        setFeedback(text, 'green');
                    });
                }
            })
            .catch(e => {
                console.error(e);
                setFeedback('API error, see console', 'red');
            });
    });
});
