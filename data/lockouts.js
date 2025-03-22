let lockoutData = [];

document.addEventListener("DOMContentLoaded", function () {
    const filterAuto = document.getElementById("filter-auto");
    const filterManual = document.getElementById("filter-manual");
    const filterX = document.getElementById("filter-x");
    const filterK = document.getElementById("filter-k");
    const filterKa = document.getElementById("filter-ka");
  
    function filterTable() {
      const rows = document.querySelectorAll('#lockoutTable tbody tr');
  
      rows.forEach(row => {
        const isAuto = row.classList.contains('auto');
        const isManual = row.classList.contains('manual');
        const kaBand = row.classList.contains('Ka');
        const kBand = row.classList.contains('K');
        const xBand = row.classList.contains('X');
  
        const matchesType = (filterAuto.checked && isAuto) || (filterManual.checked && isManual);
        const matchesBand = (filterK.checked && kBand) || (filterKa.checked && kaBand) || (filterX.checked && xBand);
  
        if (matchesType && matchesBand) {
          row.style.display = ''; // Show
        } else {
          row.style.display = 'none'; // Hide
        }
      });
    }
  
    filterAuto.addEventListener('change', filterTable);
    filterManual.addEventListener('change', filterTable);
    filterX.addEventListener('change', filterTable);
    filterK.addEventListener('change', filterTable);
    filterKa.addEventListener('change', filterTable);
  
    filterTable();
  });

document.addEventListener("DOMContentLoaded", function () {

    async function fetchLockouts() {
        try {
            const response = await fetch('/lockouts');
            const data = await response.json();
            console.log("Fetched lockouts:", data);
            
            if (data.lockouts && Array.isArray(data.lockouts)) {
                lockoutData = lockoutData = data.lockouts.map(entry => ({
                    ...entry,
                    band: getBandFromFrequency(entry.freq)
                }));
                populateLockoutTable(lockoutData);
                addRowClickHandlers();
            } else {
                console.error("Invalid JSON structure: 'lockouts' field missing or not an array");
            }
    
        } catch (error) {
            console.error("Failed to fetch lockouts:", error);
        }
    }

    function getBandFromFrequency(freq) {
        freq = Number(freq);

        if (freq >= 10000 && freq <= 11000) return "X";
        if (freq >= 24000 && freq <= 25000) return "K";
        if (freq >= 33000 && freq <= 36000) return "Ka";

        console.warn(`Unknown frequency: ${freq}`);
        return "Unknown";
    }

    function populateLockoutTable(lockoutData) {
        const tableBody = document.querySelector("#lockoutTable tbody");
        tableBody.innerHTML = "";
    
        lockoutData.forEach(entry => {
            const row = document.createElement("tr");
            
            row.setAttribute("data-lat", entry.lat.toFixed(6));
            row.setAttribute("data-lng", entry.lon.toFixed(6));
            row.classList.add(entry.type ? "manual" : "auto");
            row.classList.add(entry.band ? entry.band : "Unknown");
            
            row.innerHTML = `
                <td>${new Date(entry.ts * 1000).toLocaleString()}</td>
                <td>${entry.type ? "Manual" : "Auto"}</td>
                <td>${entry.lat.toFixed(6)}</td>
                <td>${entry.lon.toFixed(6)}</td>
                <td>${entry.dir ? "Rear" : "Front"}</td>
                <td>${entry.band}</td>
                <td>${entry.freq || "N/A"}</td>
                <td>${entry.spd ? `${entry.spd} mph` : "N/A"}</td>
                <td>${entry.str ? `${entry.str} dB` : "N/A"}</td>
                <td>${entry.cnt}</td>
                <td>${entry.course ? `${entry.course}°` : "N/A"}</td>
                <td>${entry.act ? "Active" : "Inactive"}</td>
            `;
    
            tableBody.appendChild(row);
        });
    }
    
    window.onload = fetchLockouts;

    function exportToCSV() {
        if (!lockoutData || !lockoutData.length) {
            console.error("No lockout data available to export.");
            return;
        }
    
        let csvContent = "data:text/csv;charset=utf-8,";
        csvContent += "Timestamp,Latitude,Longitude,Type,Frequency,Direction,Speed,Strength,Counter,Course,Status\n"; // Header row
    
        lockoutData.forEach(entry => {
            const row = [
                entry.ts ? new Date(entry.ts * 1000).toLocaleString() : "N/A",
                entry.lat?.toFixed(6) || "N/A",
                entry.lon?.toFixed(6) || "N/A",
                entry.type ? "Manual" : "Auto",
                entry.band || "N/A",
                entry.freq || "N/A",
                entry.dir ? "Rear" : "Front",
                entry.spd ? `${entry.spd} mph` : "N/A",
                entry.str ? `${entry.str} dB` : "N/A",
                entry.cnt,
                entry.course ? `${entry.course}°` : "N/A",
                entry.act ? "Active" : "Inactive"
            ].join(",");
            
            csvContent += row + "\n";
        });
    
        const encodedUri = encodeURI(csvContent);
        const link = document.createElement("a");
        link.setAttribute("href", encodedUri);
        link.setAttribute("download", "lockouts.csv");
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
    }
    

    function addRowClickHandlers() {
        const tableBody = document.querySelector('#lockoutTable tbody');
    
        tableBody.addEventListener('click', function(event) {
            const row = event.target.closest('tr');
            if (!row || row.classList.contains('map-row')) return;
    
            const lat = parseFloat(row.getAttribute('data-lat'));
            const lng = parseFloat(row.getAttribute('data-lng'));
            
            console.log("Latitude:", lat, "Longitude:", lng);
    
            let mapRow = row.nextSibling;
    
            if (mapRow && mapRow.classList.contains("map-row")) {
                mapRow.style.display = mapRow.style.display === "none" ? "table-row" : "none";
                return;
            }
    
            mapRow = document.createElement("tr");
            mapRow.classList.add("map-row");
            const mapCell = document.createElement("td");
            mapCell.colSpan = row.cells.length; // Span across all columns
    
            const mapContainer = document.createElement('div');
            mapContainer.classList.add('map-container');
            mapContainer.style.height = "300px";
            mapContainer.style.width = "100%";
            mapContainer.style.marginTop = "10px";
    
            mapCell.appendChild(mapContainer);
            mapRow.appendChild(mapCell);
            row.parentNode.insertBefore(mapRow, row.nextSibling);
            
            const map = L.map(mapContainer).setView([lat, lng], 17);
    
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
                attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
            }).addTo(map);
    
            L.marker([lat, lng]).addTo(map);
    
            const circle200 = L.circle([lat, lng], {
                color: "red",
                fillColor: "red",
                fillOpacity: 0.2,
                radius: 100
            }).addTo(map);
            circle200.bindTooltip("100m", { permanent: true, direction: "center" });
    
            const circle400 = L.circle([lat, lng], {
                color: "orange",
                fillColor: "orange",
                fillOpacity: 0.15,
                radius: 200
            }).addTo(map);
    
            const circle800 = L.circle([lat, lng], {
                color: "yellow",
                fillColor: "yellow",
                fillOpacity: 0.1,
                radius: 400
            }).addTo(map);
    
            setTimeout(() => {
                map.invalidateSize();
            }, 100);
        });
    }

    document.getElementById("exportCSV").addEventListener("click", exportToCSV);
    
});
