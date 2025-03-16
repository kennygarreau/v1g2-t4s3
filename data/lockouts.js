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
    // Example lockout data (replace with actual data source)
    const lockoutData = [
        { timestamp: "2025-03-09 12:30:45", type: "auto", lat: 40.712814, lon: -74.006041, band: "K", freq: "24.150 GHz" },
        { timestamp: "2025-03-09 13:15:22", type: "manual", lat: 34.052231, lon: -118.243704, band: "Ka", freq: "34.700 GHz" },
        { timestamp: "2025-03-09 14:05:10", type: "auto", lat: 41.878184, lon: -87.629821, band: "X", freq: "10.525 GHz" },
        { timestamp: "2025-03-09 15:30:45", type: "manual", lat: 20.712844, lon: -64.006061, band: "K", freq: "24.352 GHz" },
        { timestamp: "2025-03-09 16:15:22", type: "auto", lat: 34.052235, lon: -118.243744, band: "Ka", freq: "35.740 GHz" },
        { timestamp: "2025-03-09 16:05:10", type: "auto", lat: 51.878119, lon: -97.629885, band: "X", freq: "10.588 GHz" }
    ];

    function populateLockoutTable() {
        const tableBody = document.querySelector("#lockoutTable tbody");
        tableBody.innerHTML = "";

        lockoutData.forEach(entry => {
            const row = document.createElement("tr");

            row.setAttribute("data-lat", entry.lat.toFixed(6));
            row.setAttribute("data-lng", entry.lon.toFixed(6));
            row.classList.add(entry.type);
            row.classList.add(entry.band);
            
            row.innerHTML = `
                <td>${entry.timestamp}</td>
                <td>${entry.type}</td>
                <td>${entry.lat.toFixed(6)}</td>
                <td>${entry.lon.toFixed(6)}</td>
                <td>${entry.band}</td>
                <td>${entry.freq}</td>
            `;

            tableBody.appendChild(row);
        });
    }

    function exportToCSV() {
        let csvContent = "data:text/csv;charset=utf-8,";
        csvContent += "Timestamp,Latitude,Longitude,Band Type,Frequency\n"; // Header row

        lockoutData.forEach(entry => {
            let row = `${entry.timestamp},${entry.lat.toFixed(6)},${entry.lon.toFixed(6)},${entry.band},${entry.freq}`;
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

    document.getElementById("exportCSV").addEventListener("click", exportToCSV);

    // Populate table on page load
    populateLockoutTable();

    document.querySelectorAll('#lockoutTable tr').forEach(row => {
        row.addEventListener('click', function () {
            const lat = parseFloat(row.getAttribute('data-lat'));
            const lng = parseFloat(row.getAttribute('data-lng'));
            
            console.log("Latitude:", lat, "Longitude:", lng);

            let mapRow = row.nextSibling;

            if (mapRow && mapRow.classList?.contains("map-row")) {
                mapRow.style.display = mapRow.style.display === "none" ? "table-row" : "none";
                return;
            }
    
            mapRow = document.createElement("tr");
            mapRow.classList.add("map-row");
            const mapCell = document.createElement("td");
            mapCell.colSpan = row.cells.length; // Make it span all columns

            // Create the map container
            const mapContainer = document.createElement('div');
            mapContainer.classList.add('map-container');
            mapContainer.style.height = "300px";
            mapContainer.style.width = "100%";
            mapContainer.style.marginTop = "10px";

            // Append elements
            mapCell.appendChild(mapContainer);
            mapRow.appendChild(mapCell);
            row.parentNode.insertBefore(mapRow, row.nextSibling);
            
            // Initialize the Leaflet map inside the new container
            const map = L.map(mapContainer).setView([lat, lng], 15);
            
            // Add a tile layer to the map (you can replace with your own tiles)
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
              attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
            }).addTo(map);
            
            // Add a marker at the specified latitude and longitude
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
      });
});
