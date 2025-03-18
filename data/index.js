function toggleNavbar() {
    const navbar = document.getElementById('sideNavbar');
    navbar.classList.toggle('active');
}

document.addEventListener("DOMContentLoaded", () => {
    if (document.getElementById("map")) {

    const map = L.map('map', {
        center: [51.505, -0.09],
        zoom: 13,
        zoomControl: false
    });

    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
        maxZoom: 19,
        attribution: '© OpenStreetMap'
    }).addTo(map);
    
    L.control.zoom({
        position: 'bottomright'
    }).addTo(map);

    const marker = L.marker([51.505, -0.09]).addTo(map)
        .bindPopup("Latitude: 51.505, Longitude: -0.09")
        .openPopup();

    fetchLocation();

    function updateMap(lat, lon, hdop) {
        const newLatLng = new L.LatLng(lat, lon);
        
        let popupStyle = "";
        if (hdop < 2) {
            popupStyle = "background-color: green; color: white;";
        } else if (hdop < 5) {
            popupStyle = "background-color: yellow; color: black;";
        } else if (hdop < 10) {
            popupStyle = "background-color: orange; color: white;";
        } else {
            popupStyle = "background-color: red; color: white;";
        }
    
        marker.setLatLng(newLatLng);
        marker
            .getPopup()
            .setContent(
                `<div style="${popupStyle} padding: 5px; border-radius: 5px;">
                    Latitude: ${lat}<br>
                    Longitude: ${lon}<br>
                    HDOP: ${hdop}
                </div>`
            );
    
        marker.openPopup();
        map.setView(newLatLng, 16);
    }
    

async function fetchLocation() {
    try {
        const response = await fetch("/gps-info");
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        const gpsInfo = await response.json();
        console.log("Received GPS data:", gpsInfo);

        if (gpsInfo.batteryPercent) {
            const batteryPercent = Math.floor(gpsInfo.batteryPercent); 
            document.getElementById("battery-percentage").textContent = `Battery: ${batteryPercent}%`;
            document.getElementById("isBatteryCharging").textContent = `${gpsInfo.batteryCharging || ''}`;
        }
        if (gpsInfo.cpu) {
            const cpuPercent = Math.floor(gpsInfo.cpu);
            document.getElementById("cpu").textContent = `CPU: ${cpuPercent}%`;
        }
        if (gpsInfo.latitude && gpsInfo.longitude) {
            updateMap(gpsInfo.latitude, gpsInfo.longitude, gpsInfo.hdop);
        } else {
            console.error("Invalid GPS data received:", gpsInfo);
        }
    } catch (error) {
        console.error("Failed to fetch GPS info from ESP32:", error);
    }
}
    setInterval(fetchLocation, 5000);
    }
});

async function populateBoardInfo() {
    try {
        const response = await fetch("/board-info");
        if (!response.ok) {
            throw new Error("Network response was not ok");
        }

        const boardInfo = await response.json();

        document.getElementById("manufacturer").textContent = boardInfo.manufacturer || "Valentine Research";
        document.getElementById("model").textContent = boardInfo.model || "Valentine One";
        document.getElementById("serial").textContent = `Serial: ${boardInfo.serial}`;
        document.getElementById("software-version").textContent = `SW Ver: ${boardInfo.softwareVersion}`;

        clearConfigTable();
        populateConfigTable(boardInfo.config);
        populateConfigTable(boardInfo.sweepSettings);
    } catch (error) {
        console.error("Failed to fetch board info:", error);
    }
}

function clearConfigTable() {
    const tableBody = document.getElementById("config-table-body");
    tableBody.innerHTML = "";
}

function populateConfigTable(config) {
    const tableBody = document.getElementById("config-table-body");

    for (const [key, value] of Object.entries(config)) {
        if (typeof value === 'object' && Array.isArray(value)) {
            value.forEach((item, index) => {
                if (typeof item === 'object' && 'lowerBound' in item && 'upperBound' in item) {
                    const row = document.createElement("tr");

                    const keyCell = document.createElement("td");
                    if (key === 'sweepSections') {
                        keyCell.textContent = `Section ${index + 1}`;
                    } else if (key === 'customSweeps') {
                        keyCell.textContent = `Sweep ${index + 1}`;
                    }

                    const valueCell = document.createElement("td");
                    valueCell.textContent = `${(item.lowerBound / 1000).toFixed(3)} - ${(item.upperBound / 1000).toFixed(3)}`;

                    row.appendChild(keyCell);
                    row.appendChild(valueCell);
                    tableBody.appendChild(row);
                }
            });
        } else if (typeof value === 'object') {
            for (const [subKey, subValue] of Object.entries(value)) {
                const row = document.createElement("tr");

                const keyCell = document.createElement("td");
                keyCell.textContent = `${key} → ${subKey}`;

                const valueCell = document.createElement("td");
                valueCell.textContent = subValue;

                row.appendChild(keyCell);
                row.appendChild(valueCell);
                tableBody.appendChild(row);
            }
        } else {
            const row = document.createElement("tr");

            const keyCell = document.createElement("td");
            keyCell.textContent = key;

            const valueCell = document.createElement("td");
            valueCell.textContent = value;

            row.appendChild(keyCell);
            row.appendChild(valueCell);
            tableBody.appendChild(row);
        }
    }
}
populateBoardInfo();