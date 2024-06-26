#include <WiFi.h>
#include <WebSocketsServer.h>
#include <WebServer.h>
#include <ArduinoJson.h> // For two motors instance at once
#include <L298NX2.h>

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Document</title>
  <style>
    button {
      user-select: none;
    }
  </style>
</head>
<body>
  <div id="connectStatus"></div>
  <div>
    <button id="up1" ontouchstart="upPressed()" ontouchend="upReleased()">UP</button>
    <button id="down1" ontouchstart="downPressed()" ontouchend="downReleased()">DOWN</button>
    <button id="left1" ontouchstart="leftPressed()" ontouchend="leftReleased()">LEFT</button>
    <button id="right1" ontouchstart="rightPressed()" ontouchend="rightReleased()">RIGHT</button>
  </div><br>
  debug:
  <div id="received">none</div>
  <script>
    const connectStatus = document.getElementById("connectStatus");
    let ws = null;
    const received = document.getElementById("received");
    function connectWebSocket() {
      ws = new WebSocket(`ws://${window.location.hostname}:81`);
      ws.onopen = () => {
        connectStatus.innerHTML = "connected";
      };
      ws.onclose = () => {
        connectStatus.innerHTML = "disconnected";
      };
      ws.onmessage = (event) => {
        console.log(event.data);
        received.innerHTML = event.data;
      };
    }
    connectWebSocket()
    let controls = { upDown: 0, leftRight: 0 };
    document.addEventListener("keydown", function (event) {
      if (event.code === "ArrowUp") {
        controls.upDown++;
      }
      if (event.code === "ArrowDown") {
        controls.upDown--;
      }
    });
    document.addEventListener("keyup", function (event) {
      if (event.code === "ArrowUp") {
        controls.upDown--;
      }
      if (event.code === "ArrowDown") {
        controls.upDown++;
      }
    });
    function upPressed() {
      controls.upDown++;
      sendInput();
    }
    function upReleased() {
      controls.upDown--;
      sendInput();
    }
    function downPressed() {
      controls.upDown--;
      sendInput();
    }
    function downReleased() {
      controls.upDown++;
      sendInput();
    }
    function leftPressed() {
      controls.leftRight--;
      sendInput();
    }
    function leftReleased() {
      controls.leftRight++;
      sendInput();
    }
    function rightPressed() {
      controls.leftRight++;
      sendInput();
    }
    function rightReleased() {
      controls.leftRight--;
      sendInput();
    }
    function sendInput() {
      console.log(controls);
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify(controls));
      }
    }
  </script>
</body>
</html>
)rawliteral";

const char *ssid = "artemis";
const char *password = "artemis2";

WebSocketsServer webSocket = WebSocketsServer(81);
WebServer clientServer(80);

int yValue = 0;
int xValue = 0;
int leftVelocity = 0;
int rightVelocity = 0;

int ENA = 13;
int IN1 = 12;
int IN2 = 14;
int IN3 = 27;
int IN4 = 26;
int ENB = 25;

uint8_t numA;
void setupMotors()
{
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);
}

void moveMotors(int leftSpeed, int rightSpeed) {
  int leftPWM = map(abs(leftSpeed), 0, 1, 0, 255);
  int rightPWM = map(abs(rightSpeed), 0, 1, 0, 255);

  analogWrite(ENA, leftPWM);
  if (leftSpeed > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else if (leftSpeed < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  analogWrite(ENB, rightPWM);
  if (rightSpeed > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else if (rightSpeed < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }
}


/*
L298NX2 motorDrivers(ENA, IN1, IN2, IN3, IN4, ENB);
*/
void setup()
{
  setupMotors();
  Serial.begin(9600);
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  clientServer.on("/", HTTP_GET, []()
                  { clientServer.send_P(200, "text/html", index_html); });
  clientServer.begin();
}

void loop()
{

  clientServer.handleClient();
  webSocket.loop();

  leftVelocity = yValue + xValue;
  rightVelocity = yValue - xValue;
  moveMotors(leftVelocity, rightVelocity);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED:
  {
    numA = num;
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[%u] Connected from %s\n", num, ip.toString().c_str());
    break;
  }
  case WStype_TEXT:
  {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error)
    {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.f_str());
      return;
    }
    yValue = doc["upDown"];
    xValue = doc["leftRight"];
    break;
  }
  }
}