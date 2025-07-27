
setInterval(function ( ) 
{
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200)
      {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function () {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("rssi").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/rssi", true);
  xhttp.send();
}, 1000); //Put back 10000 for every 10 seconds update


function fetchAndUpdate(endpoint, elementId) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById(elementId).innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", endpoint, true);
  xhttp.send();
}

setInterval(function () {
  fetchAndUpdate("/temperature", "temperature");
  fetchAndUpdate("/humidity", "humidity");
  fetchAndUpdate("/rssi", "rssi");
}, 10000);

// Draw chart
fetch("/history.json")
  .then(response => response.json())
  .then(data => {
    // const labels = data.map(dp => new Date(parseInt(dp.t) * 1000).toLocaleTimeString());
    // const labels = data.map(dp => {
    //   const d = new Date(dp.t * 1000);
    //   const weekday = d.toLocaleString([], { weekday: 'short' });
    //   const month = d.toLocaleString([], { month: 'short' });
    //   const day = d.getDate();
    //   const hour = d.getHours().toString().padStart(2, '0');
    
    //   return `${weekday}, ${month} ${day}, ${hour}`;
    // });
    
    const labels = [];
    let lastDateString = "";

    data.forEach(dp => {
      const d = new Date(dp.t * 1000);
      const weekday = d.toLocaleString([], { weekday: 'short' });
      const month = d.toLocaleString([], { month: 'short' });
      const day = d.getDate();
      const hour = d.getHours().toString().padStart(2, '0');

      const dateKey = `${weekday}, ${month} ${day}`;

      if (dateKey !== lastDateString) {
        // First time this day appears
        labels.push(dateKey);
        lastDateString = dateKey;
      } else {
        // Same day — only show hour
        labels.push(hour);
      }
    });

    const temps = data.map(dp => dp.temp);
    const hums = data.map(dp => dp.hum);

    const minTemp = Math.min(...temps);
    const maxTemp = Math.max(...temps);
    const tempRange = maxTemp - minTemp;

    const yTempMin = Math.floor(minTemp / 5) * 5;
    const yTempMax = (tempRange < 5)
      ? yTempMin + 5
      : Math.ceil(maxTemp / 5) * 5;

    const minHum = Math.min(...hums);
    const maxHum = Math.max(...hums);
    const humRange = maxHum - minHum;

    const yHumMin = Math.floor(minHum / 5) * 5;
    const yHumMax = (humRange < 5)
      ? yHumMin + 5
      : Math.ceil(maxHum / 5) * 5;

    const ctx = document.getElementById("historyChart").getContext("2d");
    new Chart(ctx, {
      type: "line",
      data: {
        labels: labels,
        datasets: [
          {
            label: "🌡️ Temp (°C)",
            data: temps,
            borderColor: "red",
            backgroundColor: "rgba(255, 0, 0, 0.1)",
            yAxisID: 'yTemp',
            tension: 0.3
          },
          {
            label: "💧 Humidity (%)",
            data: hums,
            borderColor: "blue",
            backgroundColor: "rgba(0, 0, 255, 0.1)",
            yAxisID: 'yHum',
            tension: 0.3
          }
        ]
      },
      options: {
        responsive: true,
        scales: {
          x: {
            title: {
              display: true,
              text: "Time"
            }
          },
          yTemp: {
            type: 'linear',
            position: 'left',
            title: {
              display: true,
              text: "Temperature (°C)"
            },
            min: yTempMin,
            max: yTempMax,
            ticks: { stepSize: 1 }
          },
          yHum: {
            type: 'linear',
            position: 'right',
            title: {
              display: true,
              text: "Humidity (%)"
            },
            grid: {
              drawOnChartArea: false // Optional: avoid overlapping grid lines
            },
            min: yHumMin,
            max: yHumMax,
            ticks: { stepSize: 1 }
          }
        }
      }
    });
    
  });

setInterval(() => {
  const now = new Date();
  document.getElementById("systemTime").innerText = now.toLocaleString();
}, 1000);

// Update system status
function formatDuration(seconds) {

  seconds = Number(seconds); // force numeric type
  if (isNaN(seconds)) return "–";  // fallback display

  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;
  return `${h}h ${m}m ${s}s`;
}

function updateStatus() {
  fetch("/status.json")
    .then(res => res.json())
    .then(data => {
      document.getElementById("uptime").textContent = formatDuration(data.uptime);
      document.getElementById("heap").textContent = data.heap;
      document.getElementById("rssi").textContent = data.rssi;
      const used = (data.fs_used / 1024).toFixed(1);
      const total = (data.fs_total / 1024).toFixed(1);
      document.getElementById("fs").textContent = `${used} KB / ${total} KB`;
    });
}

setInterval(updateStatus, 10000);  // refresh every 10s
updateStatus(); // initial call
