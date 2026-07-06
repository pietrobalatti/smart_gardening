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

function setTextIfPresent(elementId, value) {
  const element = document.getElementById(elementId);
  if (element) element.innerHTML = value;
}

function updateSensorDisplay(data) {
  setTextIfPresent("temperature", data.temperature);
  setTextIfPresent("humidity", data.humidity);
  setTextIfPresent("pump1SoilMoisture", data.pump1SoilMoisture);
  setTextIfPresent("pump1SoilMoistureRaw", data.pump1SoilMoistureRaw);
  setTextIfPresent("pump2SoilMoisture", data.pump2SoilMoisture);
  setTextIfPresent("pump2SoilMoistureRaw", data.pump2SoilMoistureRaw);
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
  fetchSensorDisplay().catch(() => {});
  fetchAndUpdate("/rssi", "rssi");
}, 10000);

const DEFAULT_HISTORY_WINDOW_HOURS = 24;
let selectedHistoryHours = DEFAULT_HISTORY_WINDOW_HOURS;
let currentFullHistory = [];
let currentHistory = [];
let currentTankStatus = null;
let currentAutoIrrigationStatus = null;
let chartInstances = [];

function shouldUseDemoHistory() {
  return new URLSearchParams(window.location.search).has("demo");
}

function oneDecimal(value) {
  return Math.round(value * 10) / 10;
}

function buildDemoHistory() {
  const nowHour = Math.floor(Date.now() / 3600000) * 3600;
  const data = [];

  for (let hoursAgo = 95; hoursAgo >= 0; hoursAgo--) {
    const index = 95 - hoursAgo;
    const pump1WaterMinutes = (index % 24 === 8 || index % 24 === 16) ? 8 : 0;
    const pump2WaterMinutes = (index % 24 === 6 || index % 24 === 20) ? 3 : 0;
    const pump1WateringBoost = pump1WaterMinutes > 0 ? 9 : 0;
    const pump2WateringBoost = pump2WaterMinutes > 0 ? 7 : 0;

    data.push({
      t: nowHour - hoursAgo * 3600,
      temp: oneDecimal(23 + Math.sin(index / 4) * 4 + Math.max(0, index - 18) * 0.12),
      hum: oneDecimal(62 - Math.sin(index / 5) * 9),
      soil1: oneDecimal(48 - index * 0.22 + Math.sin(index / 3) * 4 + pump1WateringBoost),
      soil2: oneDecimal(42 - index * 0.18 + Math.sin(index / 4) * 3 + pump2WateringBoost),
      pump1WaterMinutes: pump1WaterMinutes,
      pump2WaterMinutes: pump2WaterMinutes
    });
  }

  return data;
}

function buildDemoTankStatus() {
  const nowHour = Math.floor(Date.now() / 3600000) * 3600;

  return {
    pump1: {
      used: 24,
      capacity: 90,
      lastFilled: nowHour - 12 * 3600
    },
    pump2: {
      used: 6,
      capacity: 30,
      lastFilled: nowHour - 8 * 3600
    }
  };
}

function defaultAutoIrrigationStatus() {
  return {
    enabled: false,
    hour: 20,
    minute: 45,
    pump1Minutes: 5,
    pump2Minutes: 2,
    activePhase: 0,
    lastRunDay: 0
  };
}

function formatClockTime(hour, minute) {
  return `${String(hour).padStart(2, '0')}:${String(minute).padStart(2, '0')}`;
}

function updateAutoIrrigationDisplay(status) {
  const autoStatus = status || defaultAutoIrrigationStatus();
  currentAutoIrrigationStatus = autoStatus;

  const button = document.getElementById("autoIrrigationToggle");
  const label = document.getElementById("autoIrrigationLabel");
  const message = document.getElementById("autoIrrigationMessage");
  if (!button || !label || !message) return;

  button.classList.toggle("active", autoStatus.enabled);
  button.setAttribute("aria-pressed", autoStatus.enabled ? "true" : "false");
  label.textContent = autoStatus.enabled ? "ON" : "OFF";

  if (autoStatus.activePhase === 1) {
    message.textContent = `running pump 1 (${autoStatus.pump1Minutes} min)`;
  } else if (autoStatus.activePhase === 2) {
    message.textContent = `running pump 2 (${autoStatus.pump2Minutes} min)`;
  } else if (autoStatus.enabled) {
    message.textContent = `enabled - next trigger at ${formatClockTime(autoStatus.hour, autoStatus.minute)}`;
  } else {
    message.textContent = "disabled";
  }
}

function loadAutoIrrigationStatus() {
  if (shouldUseDemoHistory()) {
    return Promise.resolve(currentAutoIrrigationStatus || defaultAutoIrrigationStatus());
  }

  return fetch("/autoirrigation.json")
    .then(response => {
      if (!response.ok) throw new Error("Failed to fetch automatic irrigation status");
      return response.json();
    });
}

function refreshAutoIrrigationStatus() {
  return loadAutoIrrigationStatus()
    .then(status => updateAutoIrrigationDisplay(status))
    .catch(() => {});
}

function setAutoIrrigationEnabled(enabled) {
  const button = document.getElementById("autoIrrigationToggle");
  if (button) button.disabled = true;

  if (shouldUseDemoHistory()) {
    const status = currentAutoIrrigationStatus || defaultAutoIrrigationStatus();
    status.enabled = enabled;
    status.activePhase = 0;
    updateAutoIrrigationDisplay(status);
    if (button) button.disabled = false;
    return;
  }

  fetch(`/autoirrigation?enabled=${enabled ? 1 : 0}`, { method: "POST" })
    .then(response => {
      if (!response.ok) throw new Error("Failed to update automatic irrigation");
      return response.json();
    })
    .then(status => updateAutoIrrigationDisplay(status))
    .catch(() => {})
    .finally(() => {
      if (button) button.disabled = false;
    });
}

function normalizeHistory(data) {
  return data
    .map(dp => {
      const soil1 = (dp.soil1 !== undefined) ? dp.soil1 : dp.soil;
      const soil2 = (dp.soil2 !== undefined) ? dp.soil2 : soil1;

      return {
        t: Number(dp.t),
        temp: Number(dp.temp),
        hum: Number(dp.hum),
        soil1: Number(soil1),
        soil2: Number(soil2),
        pump1WaterMinutes: Number(dp.pump1WaterMinutes || 0),
        pump2WaterMinutes: Number(dp.pump2WaterMinutes || 0)
      };
    })
    .filter(dp => Number.isFinite(dp.t));
}

function latestHistoryWindow(data, windowHours) {
  if (data.length === 0) return [];

  const latestTimestamp = Math.max(...data.map(dp => dp.t));
  const cutoffTimestamp = latestTimestamp - windowHours * 60 * 60;
  return data.filter(dp => dp.t >= cutoffTimestamp);
}

function updateHistoryWindowButtons() {
  document.querySelectorAll(".history-window-button").forEach(button => {
    const isActive = Number(button.dataset.historyHours) === selectedHistoryHours;
    button.classList.toggle("active", isActive);
    button.setAttribute("aria-pressed", isActive ? "true" : "false");
  });

  const label = document.getElementById("historyWindowLabel");
  if (label) label.textContent = selectedHistoryHours;
}

function redrawSelectedHistoryWindow() {
  currentHistory = latestHistoryWindow(currentFullHistory, selectedHistoryHours);
  updateHistoryWindowButtons();

  if (currentHistory.length === 0) return;
  drawHistoryCharts(currentHistory, currentTankStatus);
}

function selectHistoryWindow(hours) {
  selectedHistoryHours = hours;
  redrawSelectedHistoryWindow();
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

function defaultTankStatus() {
  return {
    pump1: {
      used: 0,
      capacity: 90,
      lastFilled: 0
    },
    pump2: {
      used: 0,
      capacity: 30,
      lastFilled: 0
    }
  };
}

function formatWaterMinutes(value) {
  const rounded = Math.round(Number(value || 0) * 10) / 10;
  return Number.isInteger(rounded) ? String(rounded) : rounded.toFixed(1);
}

function updateTankDisplay(tankStatus) {
  const pump1Status = tankStatus.pump1 || defaultTankStatus().pump1;
  const pump2Status = tankStatus.pump2 || defaultTankStatus().pump2;

  document.getElementById("pump1TankUsage").textContent =
    `${formatWaterMinutes(pump1Status.used)}/${formatWaterMinutes(pump1Status.capacity)} min`;
  document.getElementById("pump2TankUsage").textContent =
    `${formatWaterMinutes(pump2Status.used)}/${formatWaterMinutes(pump2Status.capacity)} min`;
}

function buildTankFillMarkers(data, lastFilled, markerValue) {
  const markers = data.map(() => null);
  const filledAt = Number(lastFilled || 0);

  if (!Number.isFinite(filledAt) || filledAt <= 0 || data.length === 0) return markers;

  const firstTimestamp = data[0].t;
  const lastTimestamp = data[data.length - 1].t;
  if (filledAt < firstTimestamp - 1800 || filledAt > lastTimestamp + 3600) return markers;

  let closestIndex = 0;
  let closestDelta = Math.abs(data[0].t - filledAt);

  data.forEach((dp, index) => {
    const delta = Math.abs(dp.t - filledAt);
    if (delta < closestDelta) {
      closestIndex = index;
      closestDelta = delta;
    }
  });

  markers[closestIndex] = markerValue;
  return markers;
}

function refillMarkerDataset(label, markers, yAxisID) {
  return {
    type: "line",
    label: label,
    data: markers,
    borderColor: "#111827",
    backgroundColor: "#111827",
    yAxisID: yAxisID,
    showLine: false,
    pointRadius: 5,
    pointHoverRadius: 7,
    pointStyle: "triangle",
    pointRotation: 180
  };
}

function hasVisibleMarker(markers) {
  return markers.some(value => value !== null);
}

function drawHistoryCharts(data, tankStatus) {
  chartInstances.forEach(chart => chart.destroy());
  chartInstances = [];

  const labels = buildHistoryLabels(data);
  const temps = data.map(dp => dp.temp);
  const hums = data.map(dp => dp.hum);
  const pump1Soils = data.map(dp => dp.soil1);
  const pump2Soils = data.map(dp => dp.soil2);
  const pump1WaterMinutes = data.map(dp => dp.pump1WaterMinutes);
  const pump2WaterMinutes = data.map(dp => dp.pump2WaterMinutes);
  const pump1Status = (tankStatus && tankStatus.pump1) || defaultTankStatus().pump1;
  const pump2Status = (tankStatus && tankStatus.pump2) || defaultTankStatus().pump2;

  const tempRange = axisRange(temps, 5, 5);
  const airPercentRange = axisRange(hums, 5, 5, 0, 100);
  const pump1SoilRange = axisRange(pump1Soils, 5, 5, 0, 100);
  const pump2SoilRange = axisRange(pump2Soils, 5, 5, 0, 100);
  const pump1WaterRange = axisRange(pump1WaterMinutes, 5, 5, 0);
  const pump2WaterRange = axisRange(pump2WaterMinutes, 5, 5, 0);
  const pump1FillMarkers = buildTankFillMarkers(data, pump1Status.lastFilled, pump1WaterRange.max);
  const pump2FillMarkers = buildTankFillMarkers(data, pump2Status.lastFilled, pump2WaterRange.max);
  const pump1Datasets = [
    {
      type: "line",
      label: "🌱 Soil moisture (%)",
      data: pump1Soils,
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
  ];
  const pump2Datasets = [
    {
      type: "line",
      label: "🌱 Soil moisture (%)",
      data: pump2Soils,
      borderColor: "green",
      backgroundColor: "rgba(0, 128, 0, 0.1)",
      yAxisID: 'ySoil',
      tension: 0.3
    },
    {
      type: "bar",
      label: "Pump 2 watering (min/hour)",
      data: pump2WaterMinutes,
      borderColor: "#0891b2",
      backgroundColor: "rgba(8, 145, 178, 0.45)",
      yAxisID: 'yWater'
    }
  ];

  if (hasVisibleMarker(pump1FillMarkers)) {
    pump1Datasets.push(refillMarkerDataset("Pump 1 tank filled", pump1FillMarkers, 'yWater'));
  }

  if (hasVisibleMarker(pump2FillMarkers)) {
    pump2Datasets.push(refillMarkerDataset("Pump 2 tank filled", pump2FillMarkers, 'yWater'));
  }

  chartInstances.push(new Chart(document.getElementById("environmentChart").getContext("2d"), {
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
  }));

  chartInstances.push(new Chart(document.getElementById("pump1Chart").getContext("2d"), {
    data: {
      labels: labels,
      datasets: pump1Datasets
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
        min: pump1SoilRange.min,
        max: pump1SoilRange.max,
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
  }));

  chartInstances.push(new Chart(document.getElementById("pump2Chart").getContext("2d"), {
    data: {
      labels: labels,
      datasets: pump2Datasets
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
        min: pump2SoilRange.min,
        max: pump2SoilRange.max,
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
        min: pump2WaterRange.min,
        max: pump2WaterRange.max,
        ticks: { stepSize: 1 }
      }
    })
  }));
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

function loadTankStatus() {
  if (shouldUseDemoHistory()) {
    return Promise.resolve(buildDemoTankStatus());
  }

  return fetch("/tankstatus.json")
    .then(response => {
      if (!response.ok) return defaultTankStatus();
      return response.json();
    })
    .catch(() => defaultTankStatus());
}

function refreshHistoryDashboard() {
  return Promise.all([loadHistory(), loadTankStatus()])
    .then(([historyData, tankStatus]) => {
      currentFullHistory = normalizeHistory(historyData);
      currentTankStatus = tankStatus;
      updateTankDisplay(tankStatus);
      redrawSelectedHistoryWindow();
    });
}

function resetTank(pumpNumber) {
  const buttonId = pumpNumber === 1 ? "resetPump1Tank" : "resetPump2Tank";
  const endpoint = pumpNumber === 1 ? "/resetpump1tank" : "/resetpump2tank";
  const button = document.getElementById(buttonId);

  button.disabled = true;

  if (shouldUseDemoHistory()) {
    const tankStatus = currentTankStatus || buildDemoTankStatus();
    const pumpKey = pumpNumber === 1 ? "pump1" : "pump2";
    tankStatus[pumpKey].used = 0;
    tankStatus[pumpKey].lastFilled = Math.floor(Date.now() / 1000);
    currentTankStatus = tankStatus;
    updateTankDisplay(tankStatus);
    if (currentHistory.length > 0) drawHistoryCharts(currentHistory, tankStatus);
    button.disabled = false;
    return;
  }

  fetch(endpoint, { method: "POST" })
    .then(response => {
      if (!response.ok) throw new Error("Failed to reset tank");
      return response.json();
    })
    .then(tankStatus => {
      currentTankStatus = tankStatus;
      updateTankDisplay(tankStatus);
      if (currentHistory.length > 0) drawHistoryCharts(currentHistory, tankStatus);
    })
    .catch(() => {})
    .finally(() => {
      button.disabled = false;
    });
}

document.getElementById("resetPump1Tank").addEventListener("click", () => resetTank(1));
document.getElementById("resetPump2Tank").addEventListener("click", () => resetTank(2));
document.getElementById("autoIrrigationToggle").addEventListener("click", () => {
  const status = currentAutoIrrigationStatus || defaultAutoIrrigationStatus();
  setAutoIrrigationEnabled(!status.enabled);
});
document.querySelectorAll(".history-window-button").forEach(button => {
  button.addEventListener("click", () => selectHistoryWindow(Number(button.dataset.historyHours)));
});
updateHistoryWindowButtons();
updateAutoIrrigationDisplay(defaultAutoIrrigationStatus());

// Draw charts
refreshHistoryDashboard()
  .catch(() => {});
refreshAutoIrrigationStatus();

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
setInterval(refreshAutoIrrigationStatus, 10000);
