document.addEventListener("DOMContentLoaded", () => {
    const colorPicker = document.getElementById("colorPicker");
    const colorValue = document.getElementById("textColor-current");

    document.getElementById("colorPicker").addEventListener("input", function() {
        const color = this.value;
        document.getElementById("textColor-current").textContent = color;
    });

let currentValues = {};

function removeWifiCredential(button) {
    button.parentElement.remove();
}

function addWifiCredential(ssid = "", password = "") {
    const wifiContainer = document.getElementById("wifiCredentialsContainer");

    const div = document.createElement("div");
    div.classList.add("wifi-credential");

    div.innerHTML = `
        <input type="text" name="ssid" placeholder="Enter SSID" value="${ssid}">
        <input type="password" name="password" placeholder="Enter password" value="${password}">
        <button type="button" onclick="removeWifiCredential(this)">Remove</button>
    `;

    wifiContainer.appendChild(div);
}
window.addWifiCredential = addWifiCredential;
window.removeWifiCredential = removeWifiCredential;

async function populateBoardInfo() {
        try {
            const response = await fetch("/board-info");
            if (!response.ok) {
                throw new Error("Network response was not ok");
            }

            const boardInfo = await response.json();

            // Save current values in memory
            currentValues = {
                brightness: boardInfo.displaySettings.brightness || "150",
                wifiMode: boardInfo.displaySettings.wifiMode || "WIFI_STA",
                ssid: boardInfo.displaySettings.ssid || "v1display",
                password: boardInfo.displaySettings.password || "password123",
                disableBLE: boardInfo.displaySettings.disableBLE || false,
                proxyBLE: boardInfo.displaySettings.proxyBLE || false,
                useV1LE: boardInfo.displaySettings.useV1LE || false,
                timezone: boardInfo.displaySettings.timezone || "UTC",
                enableGPS: boardInfo.displaySettings.enableGPS || false,
                lowSpeedThreshold: boardInfo.displaySettings.lowSpeedThreshold || 35,
                displayOrientation: boardInfo.displaySettings.displayOrientation || 0,
                textColor: boardInfo.displaySettings.textColor || "#FF0000",
                useDefaultV1Mode: boardInfo.displaySettings.useDefaultV1Mode || false,
                turnOffDisplay: boardInfo.displaySettings.turnOffDisplay || false,
                onlyDisplayBTIcon: boardInfo.displaySettings.onlyDisplayBTIcon || false,
                displayTest: boardInfo.displaySettings.displayTest || false,
                unitSystem: boardInfo.displaySettings.unitSystem || "Imperial",
                muteToGray: boardInfo.displaySettings.muteToGray || false,
                colorBars: boardInfo.displaySettings.colorBars || false,
                showBogeys: boardInfo.displaySettings.showBogeys || false,
                wifiCredentials: boardInfo.displaySettings.wifiCredentials || [],
            };

            let wifiType, orientation;

            if (boardInfo.displaySettings.wifiMode === 1) {
                wifiType = "Client";
            } else if (boardInfo.displaySettings.wifiMode === 2) {
                wifiType = "Access Point";
            } else {
                wifiType = "Unknown";
            }
            
            switch (boardInfo.displaySettings.displayOrientation) {
                case 0: orientation = "Landscape"; break;
                case 1: orientation = "Portrait"; break;
                case 2: orientation = "Landscape Inverted"; break;
                case 3: orientation = "Portrait Inverted"; break;
                default: orientation = "Unknown";
            }

            // Populate the form fields with current values
            document.getElementById("brightness").value = currentValues.brightness;
            document.getElementById("wifiMode").value = currentValues.wifiMode;
            document.getElementById("localSSID").value = currentValues.ssid;
            document.getElementById("localPW").value = currentValues.password;
            document.getElementById("disableBLE").value = currentValues.disableBLE.toString();
            document.getElementById("proxyBLE").value = currentValues.proxyBLE.toString();
            document.getElementById("useV1LE").value = currentValues.useV1LE.toString();
            document.getElementById("timezone").value = currentValues.timezone;
            document.getElementById("enableGPS").value = currentValues.enableGPS.toString();
            document.getElementById("lowSpeedThreshold").value = currentValues.lowSpeedThreshold;
            document.getElementById("displayOrientation").value = currentValues.displayOrientation;
            document.getElementById("colorPicker").value = currentValues.textColor;
            document.getElementById("useDefaultV1Mode").value = currentValues.useDefaultV1Mode;
            document.getElementById("turnOffDisplay").value = currentValues.turnOffDisplay.toString();
            document.getElementById("onlyDisplayBTIcon").value = currentValues.onlyDisplayBTIcon.toString();
            document.getElementById("displayTest").value = currentValues.displayTest.toString();
            document.getElementById("unitSystem").value = currentValues.unitSystem;
            document.getElementById("muteToGray").value = currentValues.muteToGray;
            document.getElementById("colorBars").value = currentValues.colorBars;
            document.getElementById("showBogeys").value = currentValues.showBogeys;

            const wifiContainer = document.getElementById("wifiCredentialsContainer");
            wifiContainer.innerHTML = "";
            currentValues.wifiCredentials.forEach((cred, index) => {
                addWifiCredential(cred.ssid, cred.password);
            });

        } catch (error) {
            console.error("Failed to fetch board info:", error);
        }
    }

document.getElementById("settingsForm").addEventListener("submit", function(event) {
        event.preventDefault();
        const formData = new FormData(this);
        const updatedSettings = {};

        if (formData.get("brightness") !== currentValues.brightness) {
            updatedSettings.brightness = formData.get("brightness");
        }
        if (formData.get("wifiMode") !== currentValues.wifiMode) {
            updatedSettings.wifiMode = formData.get("wifiMode");
        }
        if (formData.get("ssid") !== currentValues.ssid) {
            updatedSettings.ssid = formData.get("ssid");
        }
        if (formData.get("password") !== currentValues.password) {
            updatedSettings.password = formData.get("password");
        }
        if ((document.getElementById("disableBLE").value === "true") !== currentValues.disableBLE) {
            updatedSettings.disableBLE = document.getElementById("disableBLE").value === "true";
        }
        if ((document.getElementById("proxyBLE").value === "true") !== currentValues.proxyBLE) {
            updatedSettings.proxyBLE = document.getElementById("proxyBLE").value === "true";
        }
        if ((document.getElementById("useV1LE").value === "true") !== currentValues.useV1LE) {
            updatedSettings.useV1LE = document.getElementById("useV1LE").value === "true";
        }
        if (formData.get("timezone") !== currentValues.timezone) {
            updatedSettings.timezone = formData.get("timezone");
        }
        if ((document.getElementById("enableGPS").value === "true") !== currentValues.enableGPS) {
            updatedSettings.enableGPS = document.getElementById("enableGPS").value === "true";
        }
        if (parseInt(formData.get("lowSpeedThreshold")) !== currentValues.lowSpeedThreshold) {
            updatedSettings.lowSpeedThreshold = parseInt(formData.get("lowSpeedThreshold"));
        }
        if (parseInt(formData.get("displayOrientation")) !== currentValues.displayOrientation) {
            updatedSettings.displayOrientation = parseInt(formData.get("displayOrientation"));
        }
        if (formData.get("textColor") !== currentValues.textColor) {
            updatedSettings.textColor = formData.get("textColor");
        }
        if ((document.getElementById("useDefaultV1Mode").value === "true") !== currentValues.useDefaultV1Mode) {
            updatedSettings.useDefaultV1Mode = document.getElementById("useDefaultV1Mode").value === "true";
        }
        if ((document.getElementById("turnOffDisplay").value === "true") !== currentValues.turnOffDisplay) {
            updatedSettings.turnOffDisplay = document.getElementById("turnOffDisplay").value === "true";
        }
        if ((document.getElementById("onlyDisplayBTIcon").value === "true") !== currentValues.onlyDisplayBTIcon) {
            updatedSettings.onlyDisplayBTIcon = document.getElementById("onlyDisplayBTIcon").value === "true";
        }
        if ((document.getElementById("displayTest").value === "true") !== currentValues.displayTest) {
            updatedSettings.displayTest = document.getElementById("displayTest").value === "true";
        }
        if ((document.getElementById("muteToGray").value === "true") !== currentValues.muteToGray) {
            updatedSettings.muteToGray = document.getElementById("muteToGray").value === "true";
        }
        if ((document.getElementById("colorBars").value === "true") !== currentValues.colorBars) {
            updatedSettings.colorBars = document.getElementById("colorBars").value === "true";
        }
        if ((document.getElementById("showBogeys").value === "true") !== currentValues.showBogeys) {
            updatedSettings.showBogeys = document.getElementById("showBogeys").value === "true";
        }
        if (formData.get("unitSystem") !== currentValues.unitSystem) {
            updatedSettings.unitSystem = formData.get("unitSystem");
        }

        const wifiCredentials = [];
        document.querySelectorAll("#wifiCredentialsContainer .wifi-credential").forEach((div) => {
            const ssid = div.querySelector("input[name='ssid']").value.trim();
            const password = div.querySelector("input[name='password']").value.trim();
            
            if (ssid) {
                wifiCredentials.push({ ssid, password });
            }
        });

        if (JSON.stringify(wifiCredentials) !== JSON.stringify(currentValues.wifiCredentials)) {
            updatedSettings.wifiCredentials = wifiCredentials;
        }
    
        if (Object.keys(updatedSettings).length > 0) {
            fetch('/updateSettings', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(updatedSettings),
            })
            .then(response => response.json())
            .then(data => {
                console.log(data);
                console.log(updatedSettings);
                alert("Settings saved successfully");
                populateBoardInfo();
            })
            .catch(error => {
                console.error('Error:', error);
                alert("Error saving settings");
            });
        } else {
            alert("No changes detected.");
        }
    });
    
populateBoardInfo();

});