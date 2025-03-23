
async function fetchStats() {
    const response = await fetch('/stats');
    const data = await response.json();

    document.getElementById('cpu-freq').textContent = `${data.frequency} MHz`;
    document.getElementById('uptime').textContent = `${data.uptime} s`;
    document.getElementById('ble-rssi').textContent = `${data.bluetoothRSSI} dBm`;
    //document.getElementById('wifi-rssi').textContent = `${data.wifi-rssi} dBm`;

    const heapUsage = (1 - data.freeHeapInKB / data.totalHeap) * 100;
    const heapBar = document.getElementById('heap-usage');
    heapBar.style.width = `${heapUsage}%`;
    heapBar.textContent = `${heapUsage.toFixed(1)}%`;
    
    if (heapUsage > 90) {
        heapBar.style.backgroundColor = '#f44336'; // Red
    } else if (heapUsage > 75) {
        heapBar.style.backgroundColor = '#ffc107'; // Yellow
    } else {
        heapBar.style.backgroundColor = '#4caf50'; // Green
    }

    if (data.totalPsram > 0) {
        const psramUsage = Math.round((1 - data.freePsramInKB / data.totalPsram) * 100 * 100) / 100;
        const psramBar = document.getElementById('psram-usage');

        setGauge(psramUsage);
        //psramBar.style.width = `${psramUsage}%`;
        psramBar.textContent = `${psramUsage.toFixed(1)}%`;

        if (psramUsage > 90) {
            psramBar.style.backgroundColor = '#f44336'; // Red
        } else if (psramUsage > 75) {
            psramBar.style.backgroundColor = '#ffc107'; // Yellow
        } else {
            psramBar.style.backgroundColor = '#4caf50'; // Green
        }
    } else {
        document.getElementById('psram-usage').textContent = `No PSRAM`;
    }
}

function setGauge(value) {
    const gauge = document.getElementById('gauge');
    const label = document.getElementById('gauge-label');
    const rotation = (value / 100) * 180 - 180;
    gauge.style.transform = `rotate(${rotation}deg)`;
    label.textContent = `${value}%`;
}

setInterval(fetchStats, 2000);
fetchStats();
