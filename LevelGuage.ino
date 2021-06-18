// Absolute angle readings from Adafruit MPU6050
// Served to web page via websocket connection

// Libraries for MPU6050
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// Libraries for web page / websockets
#include <ESPmDNS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Arduino_JSON.h>

// Wifi credentials (note we will use AP mode when finished)
const char* ssid = "LEVEL";
const char* password = "apasswordforanAP";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Initialise MPU
Adafruit_MPU6050 mpu;

// JSON variables
float roll, pitch;
JSONVar angleValues;
JSONVar calibration;
JSONVar calibrationUpdate;

// Variables used for display and calculation
String message = "";
String rollValue = "0";
String pitchValue = "0";
String rollAdjust, pitchAdjust, rollDeviation, pitchDeviation, wheelbase, drawbar, temp;

// Read claibration values from files
void readCalibration() {
  File calibrationFile = SPIFFS.open("/calibration.json", "r");
  if(!calibrationFile){ 
    Serial.println("Failed to open config file for reading"); 
    return; 
  }
  // sanity check to make sure file is sort of valid
  size_t size = calibrationFile.size();
  if (size > 1024) {
    Serial.println("Calibration file too large");
    return;
  }
  // Load JSON array from file, JSON string must be single line
  //e.g. {"rollDeviation":0.5,"pitchDeviation":1.2,"rollAdjust":0,"pitchAdjust":0,"wheelbase":237}
  calibration = JSON.parse(calibrationFile.readString());
  // load into some strings s we can use them in other functions
  rollDeviation = calibration["rollDeviation"];
  pitchDeviation = calibration["pitchDeviation"];
  rollAdjust = calibration["rollAdjust"];
  pitchAdjust = calibration["pitchAdjust"];
  wheelbase = calibration["wheelbase"];
  drawbar = calibration["drawbar"];
  // close file
  calibrationFile.close();
 }

void writeCalibration(){
  File calibrationFile = SPIFFS.open("/calibration.json", "w");
  if(!calibrationFile){ 
    Serial.println("Failed to open config file for write"); 
    return; 
  }
  // write data
  calibrationFile.print(JSON.stringify(calibration));
  // close file
  calibrationFile.close();
 }

// Convert roll and pitch angle to JSON string
String setAngleValues(){
  angleValues["rollValue"] = String(roll,1);
  angleValues["pitchValue"] = String(pitch,1);
  // we pass the values used for calculations each time
  angleValues["rollDeviation"] = rollDeviation;
  angleValues["pitchDeviation"] = pitchDeviation;
  angleValues["wheelbase"] = wheelbase;  
  angleValues["drawbar"] = drawbar;
  String jsonString = JSON.stringify(angleValues);
  return jsonString;
}

void initMDNS() {
  if(!MDNS.begin("LevelGuage")) {
     Serial.println("Error starting mDNS");
     return;
  }
  MDNS.addService("http", "tcp", 80);
}

// Initialise SPIFFS - use No-OTA 2MB APP 2MB SPIFFS partition scheme
void initFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else{
   Serial.println("SPIFFS mounted successfully");
  }
}

// Start WIFI - note STAtion mode will be replaced with AP mode
void initWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  delay(1000);
  Serial.println(WiFi.softAPIP());
}

// Websocket send message to client
void notifyClients(String angleValues) {
  ws.textAll(angleValues);
}

// Websocket receive message from client
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char*)data;
    // Handle request for roll and pitch data
    if (strcmp((char*)data, "getValues") == 0) {
      // Read data from MPU
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);
      // Calculate absolute angles 
        roll = (atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI)-rollAdjust.toFloat();
        pitch = (atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z *a.acceleration.z)) *180.0 / PI)-pitchAdjust.toFloat();
      // Call function to send websockett data to client
      notifyClients(setAngleValues());
    } else {
      // Anything other than "getValues" is JSON calibration data
      // load JSON array - these values passed from web page
      calibrationUpdate = JSON.parse(message);
      // set deviation and wheelbase to data in message from client
      temp = calibrationUpdate["rollDeviation"];
      // Note the Arduino_JSON library will not let you assign one value directly
      // to another - intermediate step is required
      calibration["rollDeviation"] = temp;
      temp = calibrationUpdate["pitchDeviation"];
      calibration["pitchDeviation"] = temp;
      temp = calibrationUpdate["wheelbase"];
      calibration["wheelbase"] = temp; 
      temp = calibrationUpdate["drawbar"];
      calibration["drawbar"] = temp; 
      // Set adjustment figures to current roll and pitch value i.e. 
      // those values will now return zero, if zero angles flag set
      temp = calibrationUpdate["zeroAngles"];
      if (temp=="true") {
        sensors_event_t a, g, t;
        mpu.getEvent(&a, &g, &t);
        calibration["rollAdjust"] = String(atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI);
        calibration["pitchAdjust"] = String(atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z *a.acceleration.z)) *180.0 / PI);
      }
      // Save calibration data to file
      writeCalibration();
      // Update values to new ones
      rollDeviation = calibration["rollDeviation"];
      pitchDeviation = calibration["pitchDeviation"];
      rollAdjust = calibration["rollAdjust"];
      pitchAdjust = calibration["pitchAdjust"];
      wheelbase = calibration["wheelbase"];
      drawbar = calibration["drawbar"];
    }
  }
}

// Websocket global event handler -  note no error handling!
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      Serial.println(roll, pitch);
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

// Initialise websocket - points to event handler function
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    delay(10); // pause for serial console - only needed for debugging
  // Start up SPIFFS, WiFi then Websockets
  initFS();
  readCalibration();
  initWiFi();
  initWebSocket();
  initMDNS();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
  server.serveStatic("/", SPIFFS, "/");

  // Try to find MPU
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  // On startup output some info re the MPU to the console
  // Fine tune sensitivity by changing the range
  // Smaller is more sensitive
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  // Note Gyro not used in this app, so deg/s setting not relevant
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }
  /* Noise filter bandwidth.  The smaller the setting the less
     response to fast changes which could be noise, but data 
     update takes slightly longer.  That may be important in a 
     quadcopter but shouldn't matter here. */
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  Serial.println("");
  delay(100);

  // On startup we get the values of the sensors just so we can
  // print them to the console
  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  Serial.print("Temperature: ");
  Serial.print(t.temperature);
  Serial.println(" degC");

  Serial.println(""); 

  // Roll and pitch calculated from acceleration values
  roll = (atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI)-rollAdjust.toFloat();
  pitch = (atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z *a.acceleration.z)) *180.0 / PI)-pitchAdjust.toFloat();
  Serial.print("Roll: ");
  Serial.print(roll,1);
  Serial.print("   Pitch: ");
  Serial.print(pitch,1);
  Serial.print("\r");

  // Start web server
  server.begin();
}

// Main code doesn't do much - all processing done by 
// Javascript from client and websocket requests triggering functions
void loop() {

  ws.cleanupClients();

}
