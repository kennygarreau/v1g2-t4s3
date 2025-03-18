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
                    band: getBandFromFrequency(entry.frequency)
                }));
                populateLockoutTable(data.lockouts);
                addRowClickHandlers();
            } else {
                console.error("Invalid JSON structure: 'lockouts' field missing or not an array");
            }
    
        } catch (error) {
            console.error("Failed to fetch lockouts:", error);
        }
    }

    function getBandFromFrequency(freq) {
        if (freq >= 10000 && freq <= 11000) return "X";
        if (freq >= 24000 && freq <= 25000) return "K";
        if (freq >= 33000 && freq <= 36000) return "Ka";
        return "Unknown";
    }

    function populateLockoutTable(lockoutData) {
        const tableBody = document.querySelector("#lockoutTable tbody");
        tableBody.innerHTML = "";
    
        lockoutData.forEach(entry => {
            const row = document.createElement("tr");
            
            row.setAttribute("data-lat", entry.latitude.toFixed(6));
            row.setAttribute("data-lng", entry.longitude.toFixed(6));
            row.classList.add(entry.entryType ? "manual" : "auto");
            row.classList.add(entry.frequency ? `band-${entry.frequency}` : "unknown-band");
            
            row.innerHTML = `
                <td>${new Date(entry.timestamp * 1000).toLocaleString()}</td>
                <td>${entry.entryType ? "Manual" : "Auto"}</td>
                <td>${entry.latitude.toFixed(6)}</td>
                <td>${entry.longitude.toFixed(6)}</td>
                <td>${entry.direction ? "Rear" : "Front"}</td>
                <td>${entry.band}</td>
                <td>${entry.frequency || "N/A"}</td>
                <td>${entry.speed ? `${entry.speed} mph` : "N/A"}</td>
                <td>${entry.strength ? `${entry.strength} dB` : "N/A"}</td>
                <td>${entry.counter}</td>
                <td>${entry.course ? `${entry.course}°` : "N/A"}</td>
                <td>${entry.active ? "Active" : "Inactive"}</td>
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
                entry.timestamp ? new Date(entry.timestamp * 1000).toLocaleString() : "N/A",
                entry.latitude?.toFixed(6) || "N/A",
                entry.longitude?.toFixed(6) || "N/A",
                entry.entryType ? "Manual" : "Auto",
                entry.band || "N/A",
                entry.frequency || "N/A",
                entry.direction ? "Rear" : "Front",
                entry.speed ? `${entry.speed} mph` : "N/A",
                entry.strength ? `${entry.strength} dB` : "N/A",
                entry.counter,
                entry.course ? `${entry.course}°` : "N/A",
                entry.active ? "Active" : "Inactive"
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
            
            const map = L.map(mapContainer).setView([lat, lng], 15);
    
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
                attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
            }).addTo(map);
    
            L.marker([lat, lng]).addTo(map);
    
            const circle200 = L.circle([lat, lng], {
                color: "red",
                fillColor: "red",
                fillOpacity: 0.2,
                radius: 200
            }).addTo(map);
            circle200.bindTooltip("200m", { permanent: true, direction: "center" });
    
            const circle400 = L.circle([lat, lng], {
                color: "orange",
                fillColor: "orange",
                fillOpacity: 0.15,
                radius: 400
            }).addTo(map);
    
            const circle800 = L.circle([lat, lng], {
                color: "yellow",
                fillColor: "yellow",
                fillOpacity: 0.1,
                radius: 800
            }).addTo(map);
    
            setTimeout(() => {
                map.invalidateSize();
            }, 100);
        });
    }

    document.getElementById("exportCSV").addEventListener("click", exportToCSV);
    
});
