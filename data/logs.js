let currentLogFile = null;
let previousBufferCount = 0;
let autoRefreshInterval = null;
let activeMaps = {}; // Store active map instances

function switchTab(tab) {
    // Update tab buttons
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    event.target.closest('.tab').classList.add('active');
    
    // Update tab content
    document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
    document.getElementById(tab + '-tab').classList.add('active');
    
    // Start/stop auto-refresh based on active tab
    if (tab === 'live') {
        startAutoRefresh();
    } else {
        stopAutoRefresh();
    }
}

function startAutoRefresh() {
    if (!autoRefreshInterval) {
        loadBuffer();
        autoRefreshInterval = setInterval(loadBuffer, 2000);
    }
}

function stopAutoRefresh() {
    if (autoRefreshInterval) {
        clearInterval(autoRefreshInterval);
        autoRefreshInterval = null;
    }
}

async function loadStatus() {
    const res = await fetch('/api/status');
    const data = await res.json();
    
    document.getElementById('psram-entries').textContent = data.psram_entries;
    document.getElementById('psram-free').textContent = data.psram_free_kb + ' KB';
    document.getElementById('fs-used').textContent = 
        data.fs_used_kb + ' / ' + data.fs_total_kb + ' KB';
}

function toggleEntryMap(rowId, lat, lon, entry) {
    const mapRow = document.getElementById('map-' + rowId);
    const mapWrapper = mapRow ? mapRow.querySelector('.map-wrapper') : null;
    const icon = document.getElementById('icon-' + rowId);
    
    if (!mapRow || !mapWrapper) return;
    
    // Toggle expanded state
    const isExpanded = mapWrapper.classList.contains('expanded');
    
    if (isExpanded) {
        // Collapse
        mapWrapper.classList.remove('expanded');
        mapRow.classList.remove('expanded');
        icon.classList.remove('expanded');
        
        // Clean up map instance
        if (activeMaps[rowId]) {
            activeMaps[rowId].remove();
            delete activeMaps[rowId];
        }
    } else {
        // Expand
        mapRow.classList.add('expanded');
        mapWrapper.classList.add('expanded');
        icon.classList.add('expanded');
        
        // Initialize map after container is visible
        setTimeout(() => {
            const mapDiv = document.getElementById('leaflet-' + rowId);
            if (mapDiv && !activeMaps[rowId]) {
                console.log('Initializing map for', rowId, 'with coords:', lat, lon);
                
                const map = L.map(mapDiv, {
                    center: [lat, lon],
                    zoom: 16,
                    zoomControl: true
                });
                
                L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
                    maxZoom: 19,
                    attribution: '© OpenStreetMap'
                }).addTo(map);
                
                const freqValue = Number(entry.freq);
                let bandTitle = "Radar Alert"; // Default fallback
                if (freqValue === 3012) {
                    bandTitle = "Laser Alert";
                } else if (freqValue >= 10000 && freqValue <= 11000) {
                    bandTitle = "X-Band Alert";
                } else if (freqValue >= 23000 && freqValue <= 25000) {
                    bandTitle = "K-Band Alert";
                } else if (freqValue >= 33000 && freqValue <= 37000) {
                    bandTitle = "Ka-Band Alert";
                
                }
                const freqDisplay = freqValue === 3012 ? "Laser" : `${entry.freq} MHz`;
                const dirValue = Number(entry.dir);

                const directions = {
                    1: "Front",
                    2: "Side",
                    3: "Rear"
                };

                const dirDisplay = directions[dirValue] || entry.dir;
                const course = Number(entry.crs);
                const sectors = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"];
                const cardinal = sectors[Math.round(course / 45) % 8];

                // Add marker with entry details
                const date = new Date(entry.ts * 1000).toLocaleString();
                L.marker([lat, lon]).addTo(map)
                    .bindPopup(`
                        <div style="background-color: #4CAF50; color: white; padding: 8px; border-radius: 4px;">
                            <strong>${bandTitle}</strong><br>
                            Time: ${date}<br>
                            Location: ${lat.toFixed(6)}, ${lon.toFixed(6)}<br>
                            Speed: ${entry.spd} mph<br>
                            Course: ${cardinal}<br>
                            Frequency: ${freqDisplay}<br>
                            Strength: ${entry.str}<br>
                            Direction: ${dirDisplay}
                        </div>
                    `)
                    .openPopup();
                
                activeMaps[rowId] = map;
                
                // Multiple invalidateSize calls
                setTimeout(() => map.invalidateSize(), 100);
                setTimeout(() => map.invalidateSize(), 400);
            }
        }, 350);
    }
}

async function loadBuffer() {
    try {
        const res = await fetch('/api/buffer');
        const data = await res.json();
        
        const tbody = document.getElementById('buffer-tbody');
        
        if (data.entries.length === 0) {
            tbody.innerHTML = `
                <tr>
                    <td colspan="7" class="empty-state">
                        No entries in buffer. Waiting for radar alerts...
                    </td>
                </tr>
            `;
            previousBufferCount = 0;
            // Clean up any buffer maps
            Object.keys(activeMaps).forEach(mapId => {
                if (mapId.startsWith('buffer-')) {
                    activeMaps[mapId].remove();
                    delete activeMaps[mapId];
                }
            });
            return;
        }
        
        // Only rebuild DOM if the number of entries changed
        if (data.entries.length !== previousBufferCount) {
            // Track which maps are currently expanded
            const expandedMaps = new Set();
            Object.keys(activeMaps).forEach(mapId => {
                if (mapId.startsWith('buffer-')) {
                    expandedMaps.add(mapId);
                    // Remove the map instance before DOM rebuild
                    activeMaps[mapId].remove();
                    delete activeMaps[mapId];
                }
            });
            
            const hasNewEntries = data.entries.length > previousBufferCount;
            const newEntriesCount = hasNewEntries ? data.entries.length - previousBufferCount : 0;
            previousBufferCount = data.entries.length;
            
            tbody.innerHTML = data.entries.map((entry, index) => {
                const date = new Date(entry.ts * 1000).toLocaleString();
                const isNew = hasNewEntries && index >= (data.entries.length - newEntriesCount);
                const rowId = 'buffer-' + index;
                const isExpanded = expandedMaps.has(rowId);
                
                const freqValue = Number(entry.freq);
                const freqDisplay = freqValue === 3012 ? "Laser" : `${entry.freq} MHz`;
                const dirValue = Number(entry.dir);

                const directions = {
                    1: "Front",
                    2: "Side",
                    3: "Rear"
                };

                const dirDisplay = directions[dirValue] || entry.dir;
                const course = Number(entry.crs);
                const sectors = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"];
                const cardinal = sectors[Math.round(course / 45) % 8];
                
                return `
                    <tr ${isNew ? 'class="new-entry clickable"' : 'class="clickable"'} 
                        onclick="toggleEntryMap('${rowId}', ${entry.lat}, ${entry.lon}, ${JSON.stringify(entry).replace(/"/g, '&quot;')})">
                        <td>
                            <span class="expand-icon ${isExpanded ? 'expanded' : ''}" id="icon-${rowId}">▶</span>
                            ${date}
                        </td>
                        <td>${entry.lat.toFixed(6)}</td>
                        <td>${entry.lon.toFixed(6)}</td>
                        <td>${entry.spd} mph</td>
                        <td>${cardinal}</td>
                        <td>${freqDisplay}</td>
                        <td>${entry.str}</td>
                        <td>${dirDisplay}</td>
                    </tr>
                    <tr class="map-row ${isExpanded ? 'expanded' : ''}" id="map-${rowId}">
                        <td colspan="7" style="padding: 0;">
                            <div class="map-wrapper ${isExpanded ? 'expanded' : ''}">
                                <div id="leaflet-${rowId}" class="entry-map"></div>
                            </div>
                        </td>
                    </tr>
                `;
            }).join('');
            
            // Restore expanded maps after DOM update
            expandedMaps.forEach(mapId => {
                const index = parseInt(mapId.replace('buffer-', ''));
                if (index < data.entries.length) {
                    const entry = data.entries[index];
                    
                    // Re-initialize the map
                    setTimeout(() => {
                        const mapDiv = document.getElementById('leaflet-' + mapId);
                        if (mapDiv) {
                            console.log('Restoring map for', mapId);
                            
                            const map = L.map(mapDiv, {
                                center: [entry.lat, entry.lon],
                                zoom: 16,
                                zoomControl: true
                            });
                            
                            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
                                maxZoom: 19,
                                attribution: '© OpenStreetMap'
                            }).addTo(map);

                            const freqValue = Number(entry.freq);
                            let bandTitle = "Radar Alert"; // Default fallback
                            if (freqValue === 3012) {
                                bandTitle = "Laser Alert";
                            } else if (freqValue >= 10000 && freqValue <= 11000) {
                                bandTitle = "X-Band Alert";
                            } else if (freqValue >= 23000 && freqValue <= 25000) {
                                bandTitle = "K-Band Alert";
                            } else if (freqValue >= 33000 && freqValue <= 37000) {
                                bandTitle = "Ka-Band Alert";
                            
                            }
                            const freqDisplay = freqValue === 3012 ? "Laser" : `${entry.freq} MHz`;
                            const dirValue = Number(entry.dir);

                            const directions = {
                                1: "Front",
                                2: "Side",
                                3: "Rear"
                            };

                            const course = Number(entry.course);
                            const sectors = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"];
                            const cardinal = sectors[Math.round(course / 45) % 8];

                            const dirDisplay = directions[dirValue] || entry.dir;
                            console.log(dirValue, dirDisplay);

                            const date = new Date(entry.ts * 1000).toLocaleString();
                            L.marker([entry.lat, entry.lon]).addTo(map)
                                .bindPopup(`
                                    <div style="background-color: #4CAF50; color: white; padding: 8px; border-radius: 4px;">
                                        <strong>${bandTitle}</strong><br>
                                        Time: ${date}<br>
                                        Location: ${entry.lat.toFixed(6)}, ${entry.lon.toFixed(6)}<br>
                                        Speed: ${entry.spd} mph<br>
                                        Course: ${cardinal}<br>
                                        Frequency: ${freqDisplay}<br>
                                        Strength: ${entry.str}<br>
                                        Direction: ${dirDisplay}
                                    </div>
                                `)
                                .openPopup();
                            
                            activeMaps[mapId] = map;
                            
                            // Force size recalculation
                            setTimeout(() => map.invalidateSize(), 100);
                            setTimeout(() => map.invalidateSize(), 400);
                        }
                    }, 350);
                }
            });
        }
        // If entry count is the same, do nothing - keep existing DOM and maps intact
        
    } catch (err) {
        console.error('Failed to load buffer:', err);
    }
}

async function loadFileList() {
    const res = await fetch('/api/logs');
    const data = await res.json();
    
    const container = document.getElementById('file-list');
    if (data.files.length === 0) {
        container.innerHTML = '<p style="padding: 20px; color: #888;">No log files yet</p>';
        return;
    }
    
    container.innerHTML = data.files.map(f => `
        <div class="file-item" onclick="viewLog('${f.name}')">
            <div class="file-name">${f.name}</div>
            <div class="file-meta">
                ${f.entries} entries • ${(f.size / 1024).toFixed(1)} KB
            </div>
        </div>
    `).join('');
}

async function viewLog(filename) {
    currentLogFile = filename;
    document.getElementById('current-file').textContent = filename;
    document.getElementById('log-viewer').classList.remove('hidden');

    try {
        const res = await fetch('/api/logs/' + filename);
        const text = await res.text();
        
        const lines = text.trim().split('\n').filter(line => line.trim() !== "");

        const tbody = document.getElementById('log-tbody');
        tbody.innerHTML = lines.map((line, index) => {
            try {
                const entry = JSON.parse(line);
                const date = new Date(entry.ts * 1000).toLocaleString();
                const rowId = 'log-' + filename + '-' + index;
                
                const lat = (typeof entry.lat === 'number') ? entry.lat : 0;
                const lon = (typeof entry.lon === 'number') ? entry.lon : 0;

                return `
                    <tr class="clickable" 
                        onclick="toggleEntryMap('${rowId}', ${lat}, ${lon}, ${JSON.stringify(entry).replace(/"/g, '&quot;')})">
                        <td>
                            <span class="expand-icon" id="icon-${rowId}">▶</span>
                            ${date}
                        </td>
                        <td>${lat.toFixed(6)}</td>
                        <td>${lon.toFixed(6)}</td>
                        <td>${entry.spd || 0} mph</td>
                        <td>${entry.crs || 0}</td>
                        <td>${entry.freq || 0} MHz</td>
                        <td>${entry.str || 0}</td>
                        <td>${entry.dir || '-'}</td>
                    </tr>
                    <tr class="map-row" id="map-${rowId}">
                        <td colspan="7" style="padding: 0;">
                            <div class="map-wrapper">
                                <div id="leaflet-${rowId}" class="entry-map"></div>
                            </div>
                        </td>
                    </tr>
                `;
            } catch (e) {
                console.error("Error parsing line:", line, e);
                return "";
            }
        }).join('');
    } catch (err) {
        console.error("Failed to load log file:", err);
    }
}

function closeLogViewer() {
    // Clean up all active maps in the viewer
    Object.keys(activeMaps).forEach(mapId => {
        if (mapId.startsWith('log-')) {
            activeMaps[mapId].remove();
            delete activeMaps[mapId];
        }
    });
    
    document.getElementById('log-viewer').classList.add('hidden');
    currentLogFile = null;
}

async function flushLogs() {
    const res = await fetch('/api/flush', { method: 'POST' });
    const data = await res.json();
    alert(data.message);
    loadStatus();
    loadFileList();
    loadBuffer();
}

async function clearBuffer() {
    if (!confirm('Clear PSRAM buffer without saving? This will delete all unsaved entries!')) {
        return;
    }
    
    const res = await fetch('/api/buffer/clear', { method: 'POST' });
    const data = await res.json();
    alert(data.message);
    loadStatus();
    loadBuffer();
}

async function deleteCurrentLog() {
    if (!currentLogFile || !confirm('Delete ' + currentLogFile + '?')) return;
    
    await fetch('/api/logs/' + currentLogFile, { method: 'DELETE' });
    closeLogViewer();
    loadFileList();
}

// Initial load
loadStatus();
loadFileList();
startAutoRefresh();

// Update status every 5 seconds
setInterval(loadStatus, 5000);