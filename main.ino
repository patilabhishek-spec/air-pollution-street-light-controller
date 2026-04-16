#include <WiFi.h>
#include <WebServer.h>

// WiFi Credentials
const char* ssid = "Abhi";
const char* password = "12345678";

// Pin Definitions
#define MQ135_PIN 34
#define LDR_PIN   35
#define LED_PIN   26

#define LDR_THRESHOLD 2000  // Fixed LDR threshold

WebServer server(80);

// Pollution baseline (calibrate MQ135 once)
int pollutionBaseline = 1000;

// LED PWM variables
volatile int ledPercent = 0;      // 0–100%
unsigned long lastPWM = 0;
int pwmCounter = 0;               // 10-step PWM cycle

// --- Functions ---
String getAirQuality(int value) {
  return (value < pollutionBaseline + 200) ? "Good" : "Bad";
}

String getDayNight(int value) {
  return (value < LDR_THRESHOLD) ? "Day" : "Night";
}

// Continuous mapping of pollution to LED % with minimum cutoff
int getLEDBrightness(int pollutionIndex) {
  int minPollution = 50;   // LED starts glowing above this
  int maxPollution = 150;  // LED fully ON at this

  if (pollutionIndex <= minPollution) return 0;      // LED OFF below threshold
  if (pollutionIndex >= maxPollution) return 100;    // Full brightness above max

  // Linear mapping for values in-between
  int brightness = map(pollutionIndex, minPollution, maxPollution, 0, 100);
  return brightness;
}

// --- Web page handler ---
void handleRoot() {
  int pollutionRaw = analogRead(MQ135_PIN);
  int pollutionIndex = pollutionRaw - pollutionBaseline;
  if (pollutionIndex < 0) pollutionIndex = 0;

  int ldrValue = analogRead(LDR_PIN);

  String airCondition = getAirQuality(pollutionRaw);
  String dayNight = getDayNight(ldrValue);

  int brightnessPercent = 0;
  if (dayNight == "Night" && airCondition == "Bad") {
    brightnessPercent = getLEDBrightness(pollutionIndex);
  } else {
    brightnessPercent = 0; // OFF during day or good air
  }

  // Update global ledPercent (PWM will run continuously)
  ledPercent = brightnessPercent;

  // Web page
  String html = "<!DOCTYPE html><html><head><title>Street Light Dashboard</title>";
  html += "<meta http-equiv='refresh' content='2'>";
  html += "<style>body{font-family:Arial;background:#f4f4f4;text-align:center;} ";
  html += "h1{color:#222;} .card{background:white;padding:20px;margin:20px;display:inline-block;";
  html += "border-radius:10px;box-shadow:0px 4px 8px rgba(0,0,0,0.2);} ";
  html += "p{font-size:20px;color:#333;} </style></head><body>";
  html += "<h1>Pollution Based Street Light Control</h1>";
  html += "<div class='card'><h2>Air Condition</h2><p>" + airCondition + "</p></div>";
  html += "<div class='card'><h2>Pollution Index</h2><p>" + String(pollutionIndex) + "</p></div>";
  html += "<div class='card'><h2>Day or Night</h2><p>" + dayNight + "</p></div>";
  html += "<div class='card'><h2>LED Brightness</h2><p>" + String(brightnessPercent) + "%</p></div>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();

  // --- Continuous software PWM for LED ---
  unsigned long now = millis();
  if (now - lastPWM >= 1) { // 1ms step
    lastPWM = now;
    pwmCounter = (pwmCounter + 1) % 10; // 10-step PWM cycle
    if (pwmCounter < ledPercent / 10) digitalWrite(LED_PIN, HIGH);
    else digitalWrite(LED_PIN, LOW);
  }
}
