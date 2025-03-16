async function fetchStats() {
    const response = await fetch('/stats');
    const data = await response.json();

    document.getElementById('cpu-freq').textContent = `${data.cpu_freq} MHz`;
    document.getElementById('uptime').textContent = `${data.uptime} s`;
    document.getElementById('ble-rssi').textContent = `${data.ble-rssi} dBm`;
    document.getElementById('wifi-rssi').textContent = `${data.wifi-rssi} dBm`;

    const heapUsage = (1 - data.free_heap / 320000) * 100;
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

    if (data.total_psram > 0) {
        const psramUsage = (1 - data.free_psram / data.total_psram) * 100;
        const psramBar = document.getElementById('psram-usage');
        psramBar.style.width = `${psramUsage}%`;
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

setInterval(fetchStats, 2000);
fetchStats();
