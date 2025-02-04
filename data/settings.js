document.addEventListener("DOMContentLoaded", () => {
    const colorPicker = document.getElementById("colorPicker");
    const colorValue = document.getElementById("textColor-current");

    document.getElementById("colorPicker").addEventListener("input", function() {
        const color = this.value;
        document.getElementById("textColor-current").textContent = color;
    });

let currentValues = {};

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
                timezone: boardInfo.displaySettings.timezone || "UTC",
                enableGPS: boardInfo.displaySettings.enableGPS || false,
                lowSpeedThreshold: boardInfo.displaySettings.lowSpeedThreshold || 35,
                displayOrientation: boardInfo.displaySettings.displayOrientation || 0,
                textColor: boardInfo.displaySettings.textColor || "#FF0000",
                turnOffDisplay: boardInfo.displaySettings.turnOffDisplay || false,
                onlyDisplayBTIcon: boardInfo.displaySettings.onlyDisplayBTIcon || false,
                displayTest: boardInfo.displaySettings.displayTest || false,
                unitSystem: boardInfo.displaySettings.unitSystem || "Imperial",
            };

            let wifiType, orientation;

            if (boardInfo.displaySettings.wifiMode === "WIFI_STA") {
                wifiType = "Client";
            } else if (boardInfo.displaySettings.wifiMode === "WIFI_AP") {
                wifiType = "Access Point";
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
            document.getElementById("ssid").value = currentValues.ssid;
            document.getElementById("password").value = currentValues.password;
            document.getElementById("disableBLE").value = currentValues.disableBLE.toString();
            document.getElementById("timezone").value = currentValues.timezone;
            document.getElementById("enableGPS").value = currentValues.enableGPS.toString();
            document.getElementById("lowSpeedThreshold").value = currentValues.lowSpeedThreshold;
            document.getElementById("displayOrientation").value = currentValues.displayOrientation;
            document.getElementById("colorPicker").value = currentValues.textColor;
            document.getElementById("turnOffDisplay").value = currentValues.turnOffDisplay.toString();
            document.getElementById("onlyDisplayBTIcon").value = currentValues.onlyDisplayBTIcon.toString();
            document.getElementById("displayTest").value = currentValues.displayTest.toString();
            document.getElementById("unitSystem").value = currentValues.unitSystem;

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
        if ((document.getElementById("turnOffDisplay").value === "true") !== currentValues.turnOffDisplay) {
            updatedSettings.turnOffDisplay = document.getElementById("turnOffDisplay").value === "true";
        }
        if ((document.getElementById("onlyDisplayBTIcon").value === "true") !== currentValues.onlyDisplayBTIcon) {
            updatedSettings.onlyDisplayBTIcon = document.getElementById("onlyDisplayBTIcon").value === "true";
        }
        if ((document.getElementById("displayTest").value === "true") !== currentValues.displayTest) {
            updatedSettings.displayTest = document.getElementById("displayTest").value === "true";
        }
        if (formData.get("unitSystem") !== currentValues.unitSystem) {
            updatedSettings.unitSystem = formData.get("unitSystem");
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