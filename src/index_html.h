const char webpage[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <style>
        body {font-family: Arial; text-align: center; margin-top: 50px;}
        h1 {color: #333;} 
        .frequency-display {margin-bottom: 20px;}
        .settings, .went-status-box { 
            display: block;
            width: 300px; /* Fixed width for consistent alignment */
            margin: 0 auto 20px auto; /* Centered and with space between */
            border: 2px solid #ddd; 
            padding: 20px; 
            border-radius: 8px; 
            background-color: #f9f9f9;
        }
        label {display: block; margin: 10px 0 5px;} 
        input[type='text'] {padding: 5px; width: 50px; text-align: center;} 
        button {padding: 10px 20px; background-color: #4CAF50; color: white; border: none; cursor: pointer; margin-top: 10px;}
        .test-button {background-color: #2196F3;} /* Blue button for testing */
    </style>
    <title>ESP8266 Water Flow Sensor</title>
</head>
<body>
    <h1>Water Flow Sensor Dashboard</h1>
    <p class="frequency-display">Current flow: <span id="frequency">0</span> Hz</p>
    
    <div class="settings">
        <label for="frequencyThreshold">Frequency Threshold:</label>
        <input type="text" id="frequencyThreshold" value="">
        <label for="delay">Delay (seconds):</label>
        <input type="text" id="delay" value="">
        <br><br>
        <button onclick="saveSettings()">Save Settings</button>
    </div>

    <div class="went-status-box">
        <button class="test-button" onclick="testWentConnection()">Test Went Connection</button>
    </div>

    <script>
        function refresh() {
            fetch('/getCurrentStatus')
                .then(response => response.json())
                .then(data => {
                    const frequency = data.frequency;
                    const flowRate = (frequency / 11).toFixed(2); // Convert to L/min with 2 decimal places
                    document.getElementById('frequency').textContent = `${frequency} Hz (${flowRate} L/min)`;
                });
        }

        function saveSettings() {
            const frequencyThreshold = document.getElementById('frequencyThreshold').value;
            const delay = document.getElementById('delay').value;
            fetch('/setSettings', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    frequencyThreshold: Number(frequencyThreshold), 
                    delay: Number(delay)
                })
            });
        }

        function testWentConnection() {
            fetch('/testWentConnection')
                .then(response => {
                    if (response.ok) {
                       alert("Success.");
                    } else {
                       alert("Error: Unable to test Went connection.");
                    }
                })
        }

        setInterval(refresh, 1000);

        window.onload = function() {
            fetch('/getSettings')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('frequencyThreshold').value = data.frequencyThreshold;
                    document.getElementById('delay').value = data.delay;
                });
        }
    </script>
</body>
</html>
)rawliteral";
