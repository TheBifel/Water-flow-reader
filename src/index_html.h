const char webpage[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <style>
        body {font-family: Arial; text-align: center; margin-top: 50px;}
        h1 {color: #333;} 
        .frequency-display {margin-bottom: 20px;}
        label {display: block; margin: 10px 0 5px;} 
    </style>
    <title>ESP8266 Water Flow Sensor</title>
</head>
<body>
    <h1>Water Flow Sensor</h1>
    <p class="frequency-display">Current flow: <span id="frequency">0</span> Hz</p>
    <script>
        function refresh() {
            fetch('/getFrequency')
                .then(response => response.json())
                .then(data => {
                    const frequency = data.frequency;
                    const flowRate = (frequency / 11).toFixed(2); // Convert to L/min with 2 decimal places
                    document.getElementById('frequency').textContent = `${frequency} Hz (${flowRate} L/min)`;
                });
        }

        setInterval(refresh, 1000);
    </script>
</body>
</html>
)rawliteral";
