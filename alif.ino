#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <UniversalTelegramBot.h>

// ================= WiFi =================
const char* ssid = "iPhone";
const char* password = "alif131250";

// ================= LINE =================
const char* botToken = "8007253379:AAGp1zZZOquDyHpcaMTc4m0x7XD6qeNoY4Y";
const char* chatID = "8240716629";


// ================= Sensor =================
const int gasSensorPin = A0;
const int gasThreshold = 700;
const float tempThreshold = 35.0;

// ================= LED =================
const int ledPin = 14;

// ================= Server =================
ESP8266WebServer server(80);

// ================= Variables =================
float temp = 0;
int gasValue = 0;
bool alertSent = false;

// millis timers
unsigned long sensorTimer = 0;
unsigned long wifiTimer = 0;

const int sensorInterval = 2000; // อ่าน sensor ทุก 2 วิ
const int wifiCheckInterval = 10000;

// ================= HTML =================
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>Fire Alarm PRO</title>

<style>
body{
 background:#0b0b0b;
 color:white;
 font-family:Arial;
 text-align:center;
}

.card{
 background:#1b1b1b;
 margin:15px;
 padding:25px;
 border-radius:14px;
 font-size:22px;
}

.alert{
 color:red;
 font-weight:bold;
 font-size:26px;
}

.normal{
 color:lime;
 font-size:26px;
}
</style>
</head>

<body>

<h1>🚨 Fire Alarm PRO</h1>

<div class="card">
🌡 Temperature: <span id="temp">--</span> °C
</div>

<div class="card">
🧪 Gas: <span id="gas">--</span>
</div>

<div class="card">
Status: <span id="status" class="normal">SAFE</span>
</div>

<script>

function update(){

 fetch('/data')
 .then(r=>r.json())
 .then(d=>{

  temp.innerText = d.temp.toFixed(1);
  gas.innerText = d.gas;

  if(d.temp > d.tempThreshold || d.gas > d.gasThreshold){

    status.innerText="⚠️ DANGER";
    status.className="alert";

  }else{

    status.innerText="✅ SAFE";
    status.className="normal";
  }
 });
}

setInterval(update,1500);
update();

</script>
</body>
</html>
)rawliteral";

// ================= WiFi Reconnect =================
void checkWiFi(){

  if(WiFi.status() == WL_CONNECTED) return;

  Serial.println("WiFi Lost! Reconnecting...");

  WiFi.disconnect();
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED){
    delay(400);
    Serial.print(".");
  }

  Serial.println("\nReconnected!");
}

// ================= LINE =================
void sendTelegram(String message){

  WiFiClientSecure client;
  client.setInsecure();

  String url = "/bot" + String(botToken) +
               "/sendMessage?chat_id=" + String(chatID) +
               "&text=" + message;

  if (!client.connect("api.telegram.org", 443)){
    Serial.println("Telegram connection failed");
    return;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: api.telegram.org\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("Telegram Sent!");
}


// ================= Setup =================
void setup(){

  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED){
    delay(400);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.println(WiFi.localIP());

  // web
  server.on("/", [](){
    server.send(200,"text/html",htmlPage);
  });

  server.on("/data", [](){

    String json = "{\"temp\":" + String(temp,1) +
                  ",\"gas\":" + String(gasValue) +
                  ",\"tempThreshold\":" + String(tempThreshold) +
                  ",\"gasThreshold\":" + String(gasThreshold) + "}";

    server.send(200,"application/json",json);
  });

  server.begin();
}

// ================= LOOP =================
void loop(){

  server.handleClient();

  // ⭐ reconnect wifi
  if(millis() - wifiTimer > wifiCheckInterval){
    wifiTimer = millis();
    checkWiFi();
  }

  // ⭐ read sensor
  if(millis() - sensorTimer > sensorInterval){

    sensorTimer = millis();

    // จำลอง temp (เปลี่ยนเป็น DHT ได้)
    temp = random(260,380)/10.0;

    gasValue = analogRead(gasSensorPin);

    Serial.println("Temp: "+String(temp));
    Serial.println("Gas : "+String(gasValue));

    bool danger = temp > tempThreshold || gasValue > gasThreshold;

    digitalWrite(ledPin, danger);

    if(danger && !alertSent){

  String msg="FIRE ALERT!\n";

  if(temp > tempThreshold)
    msg+="Temp: "+String(temp)+"C\n";

  if(gasValue > gasThreshold)
    msg+="Gas: "+String(gasValue);

  sendTelegram(msg);
  alertSent = true;
}

if(!danger && alertSent){

  sendTelegram("Denger");
  alertSent = false;
}
  }
}

