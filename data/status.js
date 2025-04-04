let cpuChart, heapChart, wifiChart;

async function fetchSystemInfo() {
    const response = await fetch('/stats');
    const data = await response.json();
    const timestamp = Date.now();

    const usedHeap = data.totalHeap - data.freeHeapInKB;
    const usedPsram = data.totalPsram - data.freePsramInKB;

    document.getElementById('cpu-freq').textContent = `${data.frequency} MHz`;
    document.getElementById('cpu-boardType').textContent = `${data.boardType}`;
    document.getElementById('cpu-cores').textContent = `${data.cpuCores}`;
    document.getElementById('uptime').textContent = formatUptime(data.uptime);
    document.getElementById('total-storage').textContent = `${data.usedStorage} / ${data.totalStorage} KB`;
    document.getElementById('total-heap').textContent = `${usedHeap} / ${data.totalHeap} KB`;
    document.getElementById('total-psram').textContent = `${usedPsram} / ${data.totalPsram} KB`;

    const heapUsage = (1 - data.freeHeapInKB / data.totalHeap) * 100;

    if (cpuChart) updateChart(cpuChart, timestamp, data.cpuBusy);
    if (heapChart) updateChart(heapChart, timestamp, heapUsage);
    if (wifiChart) updateChart(wifiChart, timestamp, data.wifiRSSI);
    if (bleChart) updateChart(bleChart, timestamp, data.bluetoothRSSI);
}

function normalizeRSSI(rssi) {
    return Math.max(0, Math.min(100, (rssi + 100) * 2));
}

function updateChart(chart, timestamp, value) {

    if (chart.data.labels.length > 150) {
        chart.data.labels.shift();
        chart.data.datasets[0].data.shift();
    }
    chart.data.labels.push(new Date(timestamp));
    chart.data.datasets[0].data.push(value);
    chart.update();
}

function formatUptime(seconds) {
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;

    if (hours > 0) {
        return `${hours}h ${minutes}m ${secs}s`;
    } else if (minutes > 0) {
        return `${minutes}m ${secs}s`;
    } else {
        return `${secs}s`;
    }
}

document.addEventListener('DOMContentLoaded', function () {

    const ctxCpu = document.getElementById('cpuChart')?.getContext('2d');
    const ctxHeap = document.getElementById('heapChart')?.getContext('2d');
    const ctxWifi = document.getElementById('wifiChart')?.getContext('2d');
    const ctxBle = document.getElementById('bleChart')?.getContext('2d');
    
    const chartOptions = {
        responsive: true,
        scales: {
            x: { 
                type: 'time', 
                time: { unit: 'second' },
                position: 'bottom', 
                ticks: { autoSkip: true, maxTicksLimit: 10 } 
            },
            y: { beginAtZero: true, min: 0, max: 100 }
        }
    };

    const rssiOptions = {
        responsive: true,
        scales: {
            x: { 
                type: 'time', 
                time: { unit: 'second' },
                position: 'bottom', 
                ticks: { autoSkip: true, maxTicksLimit: 10 } 
            },
            y: { beginAtZero: true, min: -120, max: -30 }
        }
    };
    
    if (ctxCpu) {
        cpuChart = new Chart(ctxCpu, {
            type: 'line',
            data: { labels: [], datasets: [{ label: 'CPU Usage %', data: [], borderColor: '#4caf50', borderWidth: 2, 
                fill: true, backgroundColor: 'rgba(76, 175, 80, 0.3)', }] },
            options: chartOptions
        });
    } else {
        console.error("cpuChart canvas not found.");
    }

    if (ctxHeap) {
        heapChart = new Chart(ctxHeap, {
            type: 'line',
            data: { labels: [], datasets: [{ label: 'Heap Usage %', data: [], borderColor: '#ff9800', borderWidth: 2, 
                fill: true, backgroundColor: 'rgba(255, 152, 0, 0.3)', }] },
            options: chartOptions
        });
    } else {
        console.error("heapChart canvas not found.");
    }

    if (ctxBle) {
        bleChart = new Chart(ctxBle, {
            type: 'line',
            data: { labels: [], datasets: [{ label: 'BLE Strength (dBm)', data: [], borderColor: '#2196f3', borderWidth: 2, 
                fill: true, backgroundColor: 'rgba(33, 150, 243, 0.3)', }] },
            options: rssiOptions
        });
    } else {
        console.error("bleChart canvas not found.");
    }

    if (ctxWifi) {
        wifiChart = new Chart(ctxWifi, {
            type: 'line',
            data: { labels: [], datasets: [{ label: 'Wi-Fi Strength (dBm)', data: [], borderColor: '#ff0000', borderWidth: 2, 
                fill: true, backgroundColor: 'rgba(255, 0, 0, 0.3)', }] },
            options: rssiOptions
        });
    } else {
        console.error("wifiChart canvas not found.");
    }

    fetchSystemInfo();
    setInterval(fetchSystemInfo, 2000);
});