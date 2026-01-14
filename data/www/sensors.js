// WebSocket connection for real-time sensor updates
const REFRESH_INTERVAL_MS = 500;

let ws = null;
let reconnectTimer = null;

// Axis names for reference
const axisNames = ['Speed Brake', 'Throttle 1', 'Throttle 2', 'Flaps', 'Trim', 'Reverse 1', 'Reverse 2'];

function connectWebSocket() {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/ws`;
    
    ws = new WebSocket(wsUrl);
    
    ws.onopen = function() {
        console.log('WebSocket connected');
        document.getElementById('status').textContent = 'Connected';
        document.getElementById('status').style.color = 'green';
        
        // Clear any reconnect timer
        if (reconnectTimer) {
            clearTimeout(reconnectTimer);
            reconnectTimer = null;
        }
    };
    
    ws.onmessage = function(event) {
        try {
            const data = JSON.parse(event.data);
            updateSensorData(data);
        } catch (e) {
            console.error('Failed to parse WebSocket message:', e);
        }
    };
    
    ws.onerror = function(error) {
        console.error('WebSocket error:', error);
        document.getElementById('status').textContent = 'Connection error';
        document.getElementById('status').style.color = 'red';
    };
    
    ws.onclose = function() {
        console.log('WebSocket disconnected');
        document.getElementById('status').textContent = 'Disconnected - reconnecting...';
        document.getElementById('status').style.color = 'orange';
        
        // Attempt to reconnect after 2 seconds
        reconnectTimer = setTimeout(connectWebSocket, 2000);
    };
}

function updateSensorData(data) {
    // Update axes
    if (data.axisValues && data.axisCalibratedValues) {
        for (let i = 0; i < 7; i++) {
            const rawElement = document.getElementById(`axis-raw-${i}`);
            const calElement = document.getElementById(`axis-cal-${i}`);
            
            if (rawElement) {
                rawElement.textContent = data.axisValues[i] !== undefined ? data.axisValues[i] : '-';
            }
            if (calElement) {
                calElement.textContent = data.axisCalibratedValues[i] !== undefined ? data.axisCalibratedValues[i] : '-';
            }
        }
    }
    
    // Update buttons
    if (data.buttonsRawValue !== undefined) {
        const buttonValue = data.buttonsRawValue;
        for (let i = 0; i < 16; i++) {
            const btnElement = document.getElementById(`btn-${i}`);
            if (btnElement) {
                btnElement.checked = (buttonValue & (1 << i)) !== 0;
            }
        }
    }
}

// Initialize on page load
document.addEventListener('DOMContentLoaded', function() {
    connectWebSocket();
});

// Clean up on page unload
window.addEventListener('beforeunload', function() {
    if (ws) {
        ws.close();
    }
    if (reconnectTimer) {
        clearTimeout(reconnectTimer);
    }
});
