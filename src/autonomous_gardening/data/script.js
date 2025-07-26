
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
      // const labels = data.map(dp => new Date(parseInt(dp.t)).toLocaleTimeString());
      const labels = data.map(dp => new Date(parseInt(dp.t) * 1000).toLocaleTimeString());
      const temps = data.map(dp => dp.temp);
      const hums = data.map(dp => dp.hum);

      const ctx = document.getElementById("historyChart").getContext("2d");
      new Chart(ctx, {
        type: 'line',
        data: {
          labels: labels,
          datasets: [
            {
              label: '🌡️ Temp (°C)',
              data: temps,
              borderColor: 'red',
              fill: false
            },
            {
              label: '💧 Humidity (%)',
              data: hums,
              borderColor: 'blue',
              fill: false
            }
          ]
        },
        options: {
          responsive: true,
          scales: {
            x: { title: { display: true, text: 'Time' } },
            y: { title: { display: true, text: 'Value' } }
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
    //   const m = Math.floor((seconds &#37; 3600) / 60); // &#37; stand for modulo (%)
    //   const m = Math.floor((seconds &#37; 3600) / 60);
    //   const m = seconds; // &#37; stand for modulo (%)
      const s = seconds % 60;
      // const s = (seconds &#37; 60);
    //   const s = seconds;
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
