#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <time.h>
#include <AsyncTCP.h>
#include <AsyncElegantOTA.h>
#include <EasyDDNS.h>

// WiFi credentials
const char* ssid = “”; // & Micky con Pluto
const char* password = “”;

// DuckDNS credentials
const char* duckdns_domain = “”;
const char* duckdns_token = “”;

// DebugMode
bool activateDebug = false; // False to remove cache everyday

// iCal URLs
const char* ical_urls[] = {
“https://www.airbnb.it/calendar/ical/XXX”,
“https://www.airbnb.it/calendar/ical/XXX”,
“https://www.airbnb.it/calendar/ical/XXX”
};

// Room names
const char* room_names[] = {
“Cozy Place with King Bed”,
“Cozy & Comfortable Apartment”,
“Gemütliche 2-Zimmer Wohnung”
};

// Relay GPIO pins
const int ON_OFF = 19;
const int A = 18;
const int B = 5;
const int C = 17;
const int D = 16;

// Current state
String currentState = “OFF”;
String nextState = “OFF”;

bool manualMode = false;

//state room setup
bool cozyPlaceHasGuestYesterday = false;
bool cozyApartmentHasGuestYesterday = false;
bool twoZimmerHasGuestYesterday = false;
bool cozyPlaceHasGuestToday = false;
bool cozyApartmentHasGuestToday = false;
bool twoZimmerHasGuestToday = false;
bool cozyPlaceHasGuestTomorrow = false;
bool cozyApartmentHasGuestTomorrow = false;
bool twoZimmerHasGuestTomorrow = false;

// Web server
AsyncWebServer server(80);

// Variable to store the current IP
String currentIP = “”;

// HTML content
const char index_html[] PROGMEM = R”rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Room Control</title>
<style>
body {
font-family: Arial, sans-serif;
display: flex;
justify-content: center;
align-items: center;
height: 100vh;
margin: 0;
background: #f0f0f0;
}
.container {
text-align: center;
background: white;
padding: 20px;
border-radius: 10px;
box-shadow: 0 4px 8px rgba(0,0,0,0.1);
}
.top-right {
position: absolute;
top: 10px;
right: 10px;
display: flex;
flex-direction: column;
gap: 10px;
}
h1 {
margin-bottom: 20px;
}
.button {
display: inline-block;
margin: 10px;
padding: 15px 25px;
font-size: 16px;
color: white;
background-color: #007BFF;
border: none;
border-radius: 5px;
cursor: pointer;
transition: background-color 0.3s;
position: relative;

}
.button:hover {
background-color: #0056b3;
}
.dot {
height: 20px; /* 80% smaller than before */
width: 20px; /* 80% smaller than before */
border-radius: 50%;
display: inline-block;
}
#debug_dot{
position: absolute;
transform: translateX(-60%);
}
.green {
background-color: green;
}
.red {
background-color: red;
}
.state-info {
margin: 10px 0;
font-size: 18px;
color: #333;
}
</style>
</head>
<body>
<div class=”container”>
<h1>Room Control</h1>
<p class=”state-info”>Current State: <span id=”currentState”>%CURRENT_STATE%</span></p>
<p class=”state-info”>Next State: <span id=”nextState”>%NEXT_STATE%</span></p>
<p class=”state-info”>Next Change Time: <span id=”nextChangeTime”>%NEXT_CHANGE_TIME%</span></p>

<div class=”top-right”>
<div class=”debug-container”>
<button class=”button” onclick=”sendToggle(‘DEBUG’)”>DEBUG</button>
<span id=”debug_dot” class=”dot”></span>
</div>
<div class=”reboot-container”>
<button class=”button” onclick=”sendToggle(‘REBOOT’)”>REBOOT</button>
</div>
</div>
<button class=”button” onclick=”sendToggle(‘ON’)”>ON</button>
<span id=”on_dot” class=”dot”></span>
<br>
<button class=”button” onclick=”sendToggle(‘OFF’)”>OFF</button>
<span id=”off_dot” class=”dot”></span>
<br>
<button class=”button” onclick=”sendToggle(‘4H’)”>4H</button>
<span id=”4h_dot” class=”dot”></span>
<br>
<button class=”button” onclick=”sendToggle(‘8H’)”>8H</button>
<span id=”8h_dot” class=”dot”></span>
<br>
<button class=”button” onclick=”sendToggle(’16H’)”>16H</button>
<span id=”16h_dot” class=”dot”></span>
<br>
<button class=”button” onclick=”sendToggle(‘MANUAL’)”>MANUAL</button>
<span id=”manual_dot” class=”dot”></span>
</div>

<script>
let manualMode = false; // Global variable to track manual mode
let debugMode = false; // Global variable to track debug mode

function sendToggle(state) {
fetch(/toggle?state=${state}, {
method: ‘GET’,
headers: {
‘Authorization’: ‘Basic ‘ + btoa(‘admin:diocane!’)
}
})
.then(response => response.text())
.then(data => {
console.log(data);
if (state === ‘MANUAL’) {
manualMode = !manualMode; // Toggle manual mode
} else if (state === ‘OFF’) {
manualMode = false; // Disable manual mode when OFF is pressed
} else if (state === ‘DEBUG’) {
debugMode = !debugMode; // Toggle debug mode
} else if (state === ‘REBOOT’) {
location.reload(); // Reload the page to simulate reboot
}
updateDots(state); // Update the dots based on the current state and modes
});
}

function updateDots(state) {
document.getElementById(‘on_dot’).className = (state === ‘ON’ || state === ‘4H’ || state === ‘8H’|| state === ’16H’|| state === ‘MANUAL’) ? ‘dot green’ : ‘dot red’;
document.getElementById(‘off_dot’).className = (state === ‘OFF’) ? ‘dot green’ : ‘dot red’;
document.getElementById(‘4h_dot’).className = (state === ‘4H’) ? ‘dot green’ : ‘dot red’;
document.getElementById(‘8h_dot’).className = (state === ‘8H’) ? ‘dot green’ : ‘dot red’;
document.getElementById(’16h_dot’).className = (state === ’16H’) ? ‘dot green’ : ‘dot red’;
document.getElementById(‘debug_dot’).className = (state === ‘debugMode’) ? ‘dot green’ : ‘dot red’;

if (manualMode) {
document.getElementById(‘manual_dot’).className = ‘dot green’;
} else {
document.getElementById(‘manual_dot’).className = ‘dot red’;
}
// if (debugMode) {
// document.getElementById(‘debug_dot’).className = ‘dot green’;
// } else {
// document.getElementById(‘debug_dot’).className = ‘dot red’;
// }
}
// Initialize dots based on the current state
fetch(‘/currentState’, {
method: ‘GET’,
headers: {
‘Authorization’: ‘Basic ‘ + btoa(‘admin:diocane!’)
}
})
.then(response => response.json())
.then(data => {
manualMode = data.manualMode;
debugMode = data.debugMode;
updateDots(data.currentState);
document.getElementById(‘currentState’).innerText = data.currentState;
document.getElementById(‘nextState’).innerText = data.nextState;
document.getElementById(‘nextChangeTime’).innerText = data.nextChangeTime;

});

</script>
</body>
</html>
)rawliteral”;

void setup() {
Serial.begin(115200);

// Connect to WiFi
WiFi.begin(ssid, password);
while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(“.”);
}
Serial.println(“Connected to WiFi”);
delay(5000);

// Mount SPIFFS
if (!SPIFFS.begin(true)) {
Serial.println(“An Error has occurred while mounting SPIFFS”);
return;
}

// Configure GPIO pins
pinMode(ON_OFF, OUTPUT);
pinMode(A, OUTPUT);
pinMode(B, OUTPUT);
pinMode(C, OUTPUT);
pinMode(D, OUTPUT);

// Initialize relays based on the last known state
handleToggle(currentState);
Serial.print(“System rebooted. Current state: “);
Serial.println(currentState);

// Configure time for Europe/Rome timezone
configTime(3600, 3600, “pool.ntp.org”, “time.nist.gov”);
setenv(“TZ”, “CET-1CEST,M3.5.0/2,M10.5.0/3”, 1); // Set timezone for Europe/Rome
tzset();

// Basic authentication for the web server
server.on(“/”, HTTP_GET, [](AsyncWebServerRequest *request) {
if (!request->authenticate(“admin”, “diocane!”)) {
return request->requestAuthentication();
}

struct tm timeinfo;
if(!getLocalTime(&timeinfo)){
Serial.println(“Failed to obtain time”);
request->send(500, “text/plain”, “Server Error: Cannot get time”);
return;
}
int currentHour = timeinfo.tm_hour;

String html = String(index_html);
html.replace(“%CURRENT_STATE%”, currentState);
html.replace(“%NEXT_STATE%”, nextState);
html.replace(“%NEXT_CHANGE_TIME%”, getNextChangeTime(currentHour)); // Corrected function call

request->send(200, “text/html”, html);
});

server.on(“/toggle”, HTTP_GET, [](AsyncWebServerRequest *request) {
if (!request->authenticate(“admin”, “diocane!”)) {
return request->requestAuthentication();
}
if (request->hasParam(“state”)) {
String toggle = request->getParam(“state”)->value();
Serial.println(“Toggle State: ” + toggle);
if (toggle == “MANUAL”) {
manualMode = !manualMode;
Serial.print(“Manual mode is now “);
Serial.println(manualMode ? “ENABLED” : “DISABLED”);
} else if (toggle == “OFF”) {
manualMode = false; // Disable manual mode when OFF is pressed
} else if (toggle == “DEBUG”) {
activateDebug = !activateDebug;
Serial.print(“Debug mode is now “);
Serial.println(activateDebug ? “ENABLED” : “DISABLED”);
} else if (toggle == “REBOOT”) {
ESP.restart(); // Reboot the ESP
}

handleToggle(toggle);
currentState = toggle;
request->send(200, “text/plain”, “OK”);
} else {
request->send(400, “text/plain”, “Bad Request”);
}
});

server.on(“/currentState”, HTTP_GET, [](AsyncWebServerRequest *request) {
if (!request->authenticate(“admin”, “diocane!”)) {
return request->requestAuthentication();
}
struct tm timeinfo;
if (!getLocalTime(&timeinfo)) {
request->send(500, “Server Error: Failed to obtain time”);
return;
}
int currentHour = timeinfo.tm_hour;
String json = “{\”currentState\”:\”” + currentState + “\”, \”nextState\”:\”” + nextState + “\”, \”nextChangeTime\”:\”” + getNextChangeTime(currentHour) + “\”, \”manualMode\”:” + (manualMode ? “true” : “false”) + “, \”debugMode\”:” + (activateDebug ? “true” : “false”) + “}”;
request->send(200, “application/json”, json);
});

// Setup ElegantOTA
AsyncElegantOTA.begin(&server);
server.begin();

// EasyDDNS setup
EasyDDNS.service(“duckdns”);
EasyDDNS.client(duckdns_domain, duckdns_token);
EasyDDNS.onUpdate([](const char* oldIP, const char* newIP){
Serial.print(“EasyDDNS – IP Change Detected: “);
Serial.println(newIP);
currentIP = String(newIP);
});
// Check reservations
checkReservations();
// Trigger an update to print the IP address initially
EasyDDNS.update(2000);
// Check reservations for the next seven days at startup
checkReservationsForNextWeek();

 

}
void loop() {
static unsigned long lastClearTime = 0;
static unsigned long lastCheckReservations = 0;
static unsigned long lastDDNSUpdate = 0;
unsigned long currentMillis = millis(); // Declare currentMillis here

if (!manualMode) {
struct tm currentTime;
if (getLocalTime(&currentTime)) {
int currentHour = currentTime.tm_hour;

// Check reservations at specific times (1, 10, 20)
if ((currentHour == 1 || currentHour == 6 || currentHour == 11 || currentHour == 20) && (currentMillis – lastCheckReservations >= 60000)) {
lastCheckReservations = currentMillis;
checkReservations();
// Print the current active toggle
Serial.print(“Current active toggle is: “);
Serial.println(currentState);

// Calculate and print the next toggle change time
String nextChangeTime = getNextChangeTime(currentHour);
Serial.print(“Next toggle will change at: “);
Serial.println(nextChangeTime);
}

/* // Periodic check every 30 minutes
if (currentMillis – lastCheckReservations >= 1800000) { // 30 minutes interval
lastCheckReservations = currentMillis;
checkReservations();
// Print the current active toggle
//Serial.print(“Current active toggle is: “);
//Serial.println(currentState);
clearCacheAndGarbage();

// Calculate and print the next toggle change time
//String nextChangeTime = getNextChangeTime(currentHour);
//Serial.print(“Next toggle will change at: “);
//Serial.println(nextChangeTime);
}*/
}
}
// Update EasyDDNS every 30 minutes
if (millis() – lastDDNSUpdate >= 1800000) { // 30 minutes interval
Serial.println(“Updating EasyDDNS…”);
EasyDDNS.update(60000, false);
Serial.print(“Current IP: “);
Serial.println(currentIP);
lastDDNSUpdate = millis();
}

checkWiFiConnection();
}

void handleToggle(const String &state) {
if (state == “ON”) {
digitalWrite(ON_OFF, HIGH); // Activate ON_OFF relay
digitalWrite(A, HIGH); //NO 0
digitalWrite(B, HIGH); //NC 1
digitalWrite(C, HIGH); //NO 0
digitalWrite(D, HIGH); //NO 0
// A, B, C, D remain unaffected
} else if (state == “OFF”) {
digitalWrite(ON_OFF, LOW); // Deactivate ON_OFF relay
digitalWrite(A, HIGH); //NO 0
digitalWrite(B, HIGH); //NC 1
digitalWrite(C, HIGH); //NO 0
digitalWrite(D, HIGH); //NO 0
// A, B, C, D remain unaffected
} else if (state == “4H”) {
digitalWrite(ON_OFF, HIGH); // ON also active
digitalWrite(A, LOW); //NO 1
digitalWrite(B, LOW); //NC 0
digitalWrite(C, LOW); //NO 1
digitalWrite(D, LOW); //NO 1
} else if (state == “8H”) {
digitalWrite(ON_OFF, HIGH); // ON also active
digitalWrite(A, LOW); // NO 1
digitalWrite(B, LOW); //NC 0
digitalWrite(C, HIGH); //NO 0
digitalWrite(D, LOW); //NO 1
} else if (state == “16H”) {
digitalWrite(ON_OFF, HIGH); // ON not active
digitalWrite(A, HIGH); //NO 0
digitalWrite(B, HIGH); //NC 1
digitalWrite(C, HIGH); //NO 0
digitalWrite(D, HIGH); //NO 0
}
}

void checkReservationsForNextWeek() {
struct tm timeinfo;
if (!getLocalTime(&timeinfo)) {
Serial.println(“Failed to obtain time for simulation”);
return;
}

// Check reservations for each day in the next seven days
for (int day = 1; day <= 7; day++) {
timeinfo.tm_mday += 1;
mktime(&timeinfo); // Normalize the time structure
setenv(“TZ”, “CET-1CEST,M3.5.0/2,M10.5.0/3”, 1); // Set timezone for Europe/Rome
tzset();

// Format the date
char buffer[30];
strftime(buffer, sizeof(buffer), “%Y%m%d”, &timeinfo); // YYYYMMDD format
// Check reservations for each room on the given date
checkReservationsForDate(buffer);
}
}

void checkReservationsForDate(const char* date) {
for (int i = 0; i < 3; i++) {
String calendarData = getCalendarData(ical_urls[i]);
if (calendarData.length() > 0) {
//Serial.printf(“_______________________________\n”);
//Serial.printf(“Checking reservations for room: %s\n”, room_names[i]);
bool hasEvent = checkEvents(calendarData, date);
//Serial.printf(“There will be guests on %s in %s: %s\n”, date, room_names[i], hasEvent ? “Yes” : “No”);
}
else {
//Serial.println(“Failed to download calendar data”);
}
}
}

void checkReservations() {
struct tm timeinfo;
int retryCount = 0;
static unsigned long lastRetryReset = 0; // Keep track of last retry reset time

// Retry up to 5 times with a 1-minute interval
while (!getLocalTime(&timeinfo) && retryCount < 5) {
Serial.println(“Failed to obtain time, retrying in 1 minute…”);
delay(60000); // Wait for 1 minute
retryCount++;
}
if (retryCount == 5) {
Serial.println(“Failed to obtain time after 5 attempts, setting state to OFF.”);
// Default state in case of failure
currentState = “8H”;
handleToggle(currentState);
return;
}
// Reset the retry counter every hour if it has reached 5 retries
if (retryCount == 5 && millis() – lastRetryReset >= 3600000) { // 3600000 ms = 1 hour
retryCount = 0;
lastRetryReset = millis();
}
// Check for yesterday
timeinfo.tm_mday–;
mktime(&timeinfo);
char bufferYesterday[11];
strftime(bufferYesterday, sizeof(bufferYesterday), “%Y%m%d”, &timeinfo);
String yesterdayDateStr(bufferYesterday);

// Get today’s date
char bufferToday[11];
strftime(bufferToday, sizeof(bufferToday), “%Y%m%d”, &timeinfo);
String todayDateStr(bufferToday);

// Check for tomorrow
timeinfo.tm_mday++;
mktime(&timeinfo);
char bufferTomorrow[11];
strftime(bufferTomorrow, sizeof(bufferTomorrow), “%Y%m%d”, &timeinfo);
String tomorrowDateStr(bufferTomorrow);

for (int i = 0; i < 3; i++) {
String calendarData = getCalendarData(ical_urls[i]);
if (calendarData.length() > 0) {
bool hasEventToday = checkEvents(calendarData, todayDateStr);
bool hasEventTomorrow = checkEvents(calendarData, tomorrowDateStr);
bool hasEventYesterday = checkEvents(calendarData, yesterdayDateStr);

//Serial.printf(“There was be guests yesterday in %s: %s\n”, room_names[i], hasEventYesterday ? “Yes” : “No”);
//Serial.printf(“There will be guests today in %s: %s\n”, room_names[i], hasEventToday ? “Yes” : “No”);
//Serial.printf(“There will be guests tomorrow in %s: %s\n”, room_names[i], hasEventTomorrow ? “Yes” : “No”);

if (i == 0) { // Cozy Place with King Bed
cozyPlaceHasGuestYesterday = hasEventYesterday;
cozyPlaceHasGuestToday = hasEventToday;
cozyPlaceHasGuestTomorrow = hasEventTomorrow;
} else if (i == 1) { // Cozy & Comfortable Apartment
cozyApartmentHasGuestYesterday = hasEventYesterday;
cozyApartmentHasGuestToday = hasEventToday;
cozyApartmentHasGuestTomorrow = hasEventTomorrow;
} else if (i == 2) { // Gemütliche 2-Zimmer Wohnung
twoZimmerHasGuestYesterday = hasEventYesterday;
twoZimmerHasGuestToday = hasEventToday;
twoZimmerHasGuestTomorrow = hasEventTomorrow;
}
//Serial.println(“————————-“);

} else {
Serial.println(“Failed to download calendar data”);
}
}

struct tm currentTime;
getLocalTime(&currentTime);
int currentHour = currentTime.tm_hour;

String reason = “No guests”;

if ((cozyPlaceHasGuestToday && cozyApartmentHasGuestToday) && (cozyPlaceHasGuestTomorrow && cozyApartmentHasGuestTomorrow)) {
if (currentHour >= 11 && currentHour < 20) {
nextState = “4H”;
reason = “Both Cozy Place and Cozy Apartment have guests, switching 4H”;
} else {
nextState = “8H”;
reason = “Switching to 8H at 8 PM”;
}
} else if ((cozyPlaceHasGuestToday && cozyApartmentHasGuestToday) && !(cozyPlaceHasGuestYesterday && cozyApartmentHasGuestYesterday && twoZimmerHasGuestYesterday)) {
if (currentHour > 11 && currentHour <= 24) {
nextState = “OFF”;
reason = “No guests today and tomorrow, is OFF”;
}
} else if ((cozyPlaceHasGuestToday && !cozyApartmentHasGuestToday || !cozyPlaceHasGuestToday && cozyApartmentHasGuestToday) && (cozyPlaceHasGuestTomorrow && !cozyApartmentHasGuestTomorrow || !cozyPlaceHasGuestTomorrow && cozyApartmentHasGuestTomorrow)) {
if (currentHour >= 0 && currentHour <= 24) {
nextState = “8H”;
reason = “Guests today and tomorrow, switching to 8H”;
}
} else if ((cozyPlaceHasGuestToday && !cozyApartmentHasGuestToday || !cozyPlaceHasGuestToday && cozyApartmentHasGuestToday) && !(cozyPlaceHasGuestTomorrow && cozyApartmentHasGuestTomorrow && twoZimmerHasGuestTomorrow)) {
if (currentHour >= 1 && currentHour <= 11) {
nextState = “OFF”;
reason = “No guests tomorrow, turning off”;
}
} else if (twoZimmerHasGuestToday && twoZimmerHasGuestTomorrow) {
if (currentHour >= 11 && currentHour < 20) {
nextState = “4H”;
}
else {
nextState = “8H”;
reason = “Switching to 8H at 8 PM”;
}
} else if (twoZimmerHasGuestToday && !twoZimmerHasGuestTomorrow) {
if (currentHour >= 1 && currentHour <= 11) {
nextState = “OFF”;
}
} else {
if (currentHour >= 0 && currentHour <= 24) {
nextState = “OFF”;
reason = “No guests today, turning off”;
}
}

if (currentState != nextState) {

Serial.print(“Next state will be \n”);
Serial.print(nextState);
Serial.print(“\nBecause “);
Serial.println(reason);

handleToggle(nextState);
currentState = nextState;
}
}

String getCalendarData(const char* url) {
HTTPClient http;
http.begin(url);
int httpCode = http.GET();
if (httpCode == HTTP_CODE_OK) {
String payload = http.getString();
http.end();
return payload;
} else {
http.end();
return “”;
}
}

bool checkEvents(const String& calendarData, const String& date) {
int startIdx = 0;
while ((startIdx = calendarData.indexOf(“BEGIN:VEVENT”, startIdx)) != -1) {
int endIdx = calendarData.indexOf(“END:VEVENT”, startIdx) + 10; // Find the end of the current event

// Find and extract SUMMARY
int summaryIdx = calendarData.indexOf(“SUMMARY”, startIdx);
String eventSummary;
if (summaryIdx != -1 && summaryIdx < endIdx) {
int summaryStartIdx = calendarData.indexOf(‘:’, summaryIdx) + 1;
int summaryEndIdx = calendarData.indexOf(‘\r’, summaryStartIdx);
eventSummary = calendarData.substring(summaryStartIdx, summaryEndIdx);
}

// Find and extract DTSTART
int dtstartIdx = calendarData.indexOf(“DTSTART”, startIdx);
String dtstartDate;
if (dtstartIdx != -1 && dtstartIdx < endIdx) {
int dtstartValIdx = calendarData.indexOf(‘:’, dtstartIdx) + 1;
int dtstartEndIdx = calendarData.indexOf(‘\r’, dtstartValIdx);
dtstartDate = calendarData.substring(dtstartValIdx, dtstartEndIdx);
dtstartDate = dtstartDate.substring(0, 8); // Extract only the first 8 characters
}

// Find and extract DTEND
int dtendIdx = calendarData.indexOf(“DTEND”, startIdx);
String dtendDate;
if (dtendIdx != -1 && dtendIdx < endIdx) {
int dtendValIdx = calendarData.indexOf(‘:’, dtendIdx) + 1;
int dtendEndIdx = calendarData.indexOf(‘\r’, dtendValIdx);
dtendDate = calendarData.substring(dtendValIdx, dtendEndIdx);
dtendDate = dtendDate.substring(0, 8); // Extract only the first 8 characters
}

// Check if date is within the event period and summary contains “Reserved”
if (eventSummary.indexOf(“Reserved”) != -1 && date >= dtstartDate && date < dtendDate) {
if (date == dtstartDate) {
//Serial.println(“Event is starting on this date”);
}
if (date == dtendDate) {
//Serial.println(“Event is finishing on this date”);
}
return true; // Event found
}

// Move to the next event
startIdx = endIdx;
}
return false; // No events found that match criteria
}

void checkWiFiConnection() {
static unsigned long lastCheck = 0;
if (millis() – lastCheck >= 1800000) { // 30 minutes in milliseconds
if (WiFi.status() != WL_CONNECTED) {
Serial.println(“Reconnecting to WiFi…”);
WiFi.disconnect();
WiFi.begin(ssid, password);
while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(“.”);
}
Serial.println(“Reconnected to WiFi”);
}
lastCheck = millis();
}
}

String getNextChangeTime(int currentHour) {
struct tm currentTime;
getLocalTime(&currentTime);
currentTime.tm_min = 0;
currentTime.tm_sec = 0;

if (currentHour >= 1 && currentHour < 11) {
currentTime.tm_hour = 11;
} else if (currentHour >= 11 && currentHour < 20 ) {
currentTime.tm_hour = 20;
} else if (currentHour > 20 && currentHour <= 24) {
currentTime.tm_hour = 1;
currentTime.tm_mday += 1;
}

//time_t nextChangeTime = mktime(&currentTime);
char buffer[30];
strftime(buffer, sizeof(buffer), “%Y-%m-%d %H:%M:%S”, &currentTime);
return String(buffer);
}

void clearCacheAndGarbage() {
Serial.println(“Clearing cache and garbage…”);

// This example clears all files in SPIFFS
File root = SPIFFS.open(“/”);
File file = root.openNextFile();

while (file) {
String fileName = file.name();
Serial.print(“Deleting file: “);
Serial.println(fileName);
SPIFFS.remove(fileName);
file = root.openNextFile();
}

Serial.println(“Cache and garbage cleared.”);
}