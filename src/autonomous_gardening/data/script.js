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

function updateSensorDisplay(data) {
  document.getElementById("temperature").innerHTML = data.temperature;
  document.getElementById("humidity").innerHTML = data.humidity;
  document.getElementById("soilMoisture").innerHTML = data.soilMoisture;
  document.getElementById("soilMoistureRaw").innerHTML = data.soilMoistureRaw;
}

function fetchSensorDisplay() {
  return fetch("/sensors.json")
    .then(response => {
      if (!response.ok) throw new Error("Failed to fetch sensors");
      return response.json();
    })
    .then(data => {
      updateSensorDisplay(data);
      return data;
    });
}

function pollSensorRefresh(requestMillis, attempt) {
  return fetchSensorDisplay()
    .then(data => {
      const refreshComplete = !data.refreshPending && data.lastRefreshMillis >= requestMillis;

      if (refreshComplete) {
        return data;
      }

      if (attempt >= 20) {
        throw new Error("Sensor refresh timed out");
      }

      return new Promise(resolve => setTimeout(resolve, 500))
        .then(() => pollSensorRefresh(requestMillis, attempt + 1));
    });
}

function refreshSensors() {
  var button = document.getElementById("refreshSensors");
  var status = document.getElementById("refreshStatus");

  button.disabled = true;
  status.innerHTML = "refreshing...";

  fetch("/refreshsensors")
    .then(response => {
      if (!response.ok) throw new Error("Failed to request sensor refresh");
      return response.json();
    })
    .then(data => pollSensorRefresh(data.requestMillis, 0))
    .then(data => {
      updateSensorDisplay(data);
      status.innerHTML = "updated";
    })
    .catch(() => {
      status.innerHTML = "failed";
    })
    .finally(() => {
      button.disabled = false;
    });
}

document.getElementById("refreshSensors").addEventListener("click", refreshSensors);

setInterval(function () {
  fetchAndUpdate("/temperature", "temperature");
  fetchAndUpdate("/humidity", "humidity");
  fetchAndUpdate("/soilmoisture", "soilMoisture");
  fetchAndUpdate("/soilmoistureraw", "soilMoistureRaw");
  fetchAndUpdate("/rssi", "rssi");
}, 10000);

const HISTORY_WINDOW_HOURS = 24;

function shouldUseDemoHistory() {
  return new URLSearchParams(window.location.search).has("demo");
}

function oneDecimal(value) {
  return Math.round(value * 10) / 10;
}

function buildDemoHistory() {
  const nowHour = Math.floor(Date.now() / 3600000) * 3600;
  const data = [];

  for (let hoursAgo = 29; hoursAgo >= 0; hoursAgo--) {
    const index = 29 - hoursAgo;
    const pump1WaterMinutes = (index === 8 || index === 16 || index === 24) ? 8 : 0;
    const pump2WaterMinutes = (index === 6 || index === 20) ? 3 : 0;
    const wateringBoost = pump1WaterMinutes > 0 ? 9 : 0;

    data.push({
      t: nowHour - hoursAgo * 3600,
      temp: oneDecimal(23 + Math.sin(index / 4) * 4 + Math.max(0, index - 18) * 0.12),
      hum: oneDecimal(62 - Math.sin(index / 5) * 9),
      soil: oneDecimal(48 - index * 0.22 + Math.sin(index / 3) * 4 + wateringBoost),
      pump1WaterMinutes: pump1WaterMinutes,
      pump2WaterMinutes: pump2WaterMinutes
    });
  }

  return data;
}

function normalizeHistory(data) {
  return data
    .map(dp => ({
      t: Number(dp.t),
      temp: Number(dp.temp),
      hum: Number(dp.hum),
      soil: Number(dp.soil),
      pump1WaterMinutes: Number(dp.pump1WaterMinutes || 0),
      pump2WaterMinutes: Number(dp.pump2WaterMinutes || 0)
    }))
    .filter(dp => Number.isFinite(dp.t));
}

function latestHistoryWindow(data) {
  if (data.length === 0) return [];

  const latestTimestamp = Math.max(...data.map(dp => dp.t));
  const cutoffTimestamp = latestTimestamp - HISTORY_WINDOW_HOURS * 60 * 60;
  return data.filter(dp => dp.t >= cutoffTimestamp);
}

function buildHistoryLabels(data) {
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
      labels.push(dateKey);
      lastDateString = dateKey;
    } else {
      labels.push(hour);
    }
  });

  return labels;
}

function axisRange(values, minSpan, stepSize, minLimit, maxLimit) {
  const finiteValues = values.filter(value => Number.isFinite(value));
  if (finiteValues.length === 0) {
    return { min: minLimit || 0, max: (minLimit || 0) + minSpan };
  }

  const minValue = Math.min(...finiteValues);
  const maxValue = Math.max(...finiteValues);
  let axisMin = Math.floor(minValue / stepSize) * stepSize;
  let axisMax = (maxValue - minValue < minSpan)
    ? axisMin + minSpan
    : Math.ceil(maxValue / stepSize) * stepSize;

  if (minLimit !== undefined) axisMin = Math.max(minLimit, axisMin);
  if (maxLimit !== undefined) axisMax = Math.min(maxLimit, axisMax);
  if (axisMax <= axisMin) {
    if (maxLimit !== undefined && axisMin + minSpan > maxLimit) {
      axisMax = maxLimit;
      axisMin = axisMax - minSpan;
      if (minLimit !== undefined) axisMin = Math.max(minLimit, axisMin);
    } else {
      axisMax = axisMin + minSpan;
    }
  }

  return { min: axisMin, max: axisMax };
}

function sharedChartOptions(scales) {
  return {
    responsive: true,
    maintainAspectRatio: false,
    interaction: {
      mode: 'index',
      intersect: false
    },
    scales: scales
  };
}

function drawHistoryCharts(data) {
  const labels = buildHistoryLabels(data);
  const temps = data.map(dp => dp.temp);
  const hums = data.map(dp => dp.hum);
  const soils = data.map(dp => dp.soil);
  const pump1WaterMinutes = data.map(dp => dp.pump1WaterMinutes);
  const pump2WaterMinutes = data.map(dp => dp.pump2WaterMinutes);

  const tempRange = axisRange(temps, 5, 5);
  const airPercentRange = axisRange(hums, 5, 5, 0, 100);
  const soilRange = axisRange(soils, 5, 5, 0, 100);
  const pump1WaterRange = axisRange(pump1WaterMinutes, 5, 5, 0);
  const pump2WaterRange = axisRange(pump2WaterMinutes, 5, 5, 0);

  new Chart(document.getElementById("environmentChart").getContext("2d"), {
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
          yAxisID: 'yPercent',
          tension: 0.3
        }
      ]
    },
    options: sharedChartOptions({
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
        min: tempRange.min,
        max: tempRange.max,
        ticks: { stepSize: 1 }
      },
      yPercent: {
        type: 'linear',
        position: 'right',
        title: {
          display: true,
          text: "Humidity (%)"
        },
        grid: {
          drawOnChartArea: false
        },
        min: airPercentRange.min,
        max: airPercentRange.max,
        ticks: { stepSize: 1 }
      }
    })
  });

  new Chart(document.getElementById("pump1Chart").getContext("2d"), {
    data: {
      labels: labels,
      datasets: [
        {
          type: "line",
          label: "🌱 Soil moisture (%)",
          data: soils,
          borderColor: "green",
          backgroundColor: "rgba(0, 128, 0, 0.1)",
          yAxisID: 'ySoil',
          tension: 0.3
        },
        {
          type: "bar",
          label: "Pump 1 watering (min/hour)",
          data: pump1WaterMinutes,
          borderColor: "#d97706",
          backgroundColor: "rgba(217, 119, 6, 0.45)",
          yAxisID: 'yWater'
        }
      ]
    },
    options: sharedChartOptions({
      x: {
        title: {
          display: true,
          text: "Time"
        }
      },
      ySoil: {
        type: 'linear',
        position: 'left',
        title: {
          display: true,
          text: "Soil moisture (%)"
        },
        min: soilRange.min,
        max: soilRange.max,
        ticks: { stepSize: 1 }
      },
      yWater: {
        type: 'linear',
        position: 'right',
        title: {
          display: true,
          text: "Watering (min/hour)"
        },
        grid: {
          drawOnChartArea: false
        },
        min: pump1WaterRange.min,
        max: pump1WaterRange.max,
        ticks: { stepSize: 1 }
      }
    })
  });

  new Chart(document.getElementById("pump2Chart").getContext("2d"), {
    type: "bar",
    data: {
      labels: labels,
      datasets: [
        {
          label: "Pump 2 watering (min/hour)",
          data: pump2WaterMinutes,
          borderColor: "#0891b2",
          backgroundColor: "rgba(8, 145, 178, 0.45)"
        }
      ]
    },
    options: sharedChartOptions({
      x: {
        title: {
          display: true,
          text: "Time"
        }
      },
      y: {
        type: 'linear',
        position: 'left',
        title: {
          display: true,
          text: "Watering (min/hour)"
        },
        min: pump2WaterRange.min,
        max: pump2WaterRange.max,
        ticks: { stepSize: 1 }
      }
    })
  });
}

function loadHistory() {
  if (shouldUseDemoHistory()) {
    return Promise.resolve(buildDemoHistory());
  }

  return fetch("/history.json")
    .then(response => {
      if (!response.ok) return [];
      return response.json();
    });
}

// Draw charts
loadHistory()
  .then(data => {
    const history = latestHistoryWindow(normalizeHistory(data));
    if (history.length === 0) return;

    drawHistoryCharts(history);
  })
  .catch(() => {});

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
      const heapElement = document.getElementById("heap");
      if (heapElement) heapElement.textContent = data.heap;
      document.getElementById("rssi").textContent = data.rssi;
      const used = (data.fs_used / 1024).toFixed(1);
      const total = (data.fs_total / 1024).toFixed(1);
      document.getElementById("fs").textContent = `${used} KB / ${total} KB`;
    })
    .catch(() => {});
}

setInterval(updateStatus, 10000);  // refresh every 10s
updateStatus(); // initial call
