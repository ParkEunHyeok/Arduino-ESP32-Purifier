#include <WiFi.h> // 와이파이
#include <HTTPClient.h> // 프로그램 저장공간 17% 차지
#include "ESPAsyncWebServer.h" // 센싱장치가 웹 서버 열라고 씀
#include <Arduino_JSON.h> // 우리 동네 미세먼지값을 JSON 형식으로 읽어옴
#include <AsyncTCP.h> // 서버에다가 요청해서 값을 읽어온 후 서버의 정보와 일치한지 검사해줌.
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel strip = Adafruit_NeoPixel(3, 12, NEO_GRB + NEO_KHZ800);

const char* PARAM_INPUT_1 = "state";

// 접속할 공유기의 ssid, 비밀번호
const char* ssid = "olleh_WiFi_215A";
const char* password = "000000486a";
const String endpoint = "http://openapi.airkorea.or.kr/openapi/services/rest/ArpltnInforInqireSvc/getMsrstnAcctoRltmMesureDnsty?stationName=%EC%A2%85%EB%A1%9C%EA%B5%AC&dataTerm=month&pageNo=1&numOfRows=10&ServiceKey=bytN9iG7InXPwI3ETOzsh%2Fqy%2BuxCNmHpG9PeFo%2FNSP92e9UjO6CYS7PzKRD%2Fyijj5rOONKhpWrAMc4jyyZ4ktw%3D%3D&ver=1.3";
String line = "";

const int output = 2;
const int buttonPin = 4;
int ledState = LOW;          // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

unsigned long lastTime = 0;  
unsigned long timerDelay = 3000;  // send readings timer
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

// web 서버를 만들고 포트를 80으로 설정한다.
AsyncWebServer server(80);
AsyncEventSource events("/events");

int microdust_pm10;
// 공공 데이터 포털에서 대기오염정보를 파싱해오기 위해서 필요한 함수. 초미세먼지는 파싱 안하고 지금은 미세먼지만 해놓음
void get_weather(void) {
  if ((WiFi.status() == WL_CONNECTED)) { //와이파이에 연결되어 있다면
    Serial.println("Starting connection to server...");
    // http client
    HTTPClient http;
    http.begin(endpoint);       //Specify the URL
    int httpCode = http.GET();  //Make the request
    if (httpCode > 0) {         //Check for the returning code
      line = http.getString();
    }
    else {
      Serial.println("Error on HTTP request");
    }
//    Serial.println(line); // 수신한 날씨 정보 시리얼 모니터 출력
    String pm10;  // 문자열을 만들음.
    int pm10Value_start= line.indexOf(F("<pm10Value>")); // "<tm>"문자가 시작되는 인덱스 값('<'의 인덱스)을 반환한다. 
    int pm10Value_end= line.indexOf(F("</pm10Value>"));  
    pm10 = line.substring(pm10Value_start + 11, pm10Value_end); // +1: "<tm>"스트링의 크기 11바이트, 11칸 이동
    Serial.print(F("pm10: ")); Serial.println(pm10); // 시리얼 모니터에 pm10(미세먼지)의 값을 표시함.
    
    int inString1 = pm10.substring(0, pm10.length()).toInt(); // inString1이라는 정수로 반환함.
    Serial.println(inString1);
    if(0<inString1 && inString1<=30) { // 미세먼지의 측정값을 통해서 등급을 매김. 표준
      Serial.println("good"); // 좋음, 초록
      colorWipe(strip.Color(0,0,255), 0);
      digitalWrite(27, LOW);
    } else if(30<inString1 && inString1<=80) { 
      Serial.println("soso"); // 보통, 파랑
      colorWipe(strip.Color(0,255,0), 0);
      digitalWrite(27, LOW);
    }else if(80<inString1 && inString1<=150) { 
      Serial.println("bad"); // 나쁨, 노랑
      colorWipe(strip.Color(255,255,0), 0);
      digitalWrite(27, HIGH);
    } else { 
      Serial.println("very bad!"); // 매우 나쁨, 빨강
      colorWipe(strip.Color(255,0,0), 0);
      digitalWrite(27, HIGH);
    }
    
    http.end(); //Free the resources
    microdust_pm10 = inString1;
  }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Air Care System Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <!--  버튼 javascript -->
  <style>
    html {
      font-family: Arial;
      display: inline-block;
      text-align: center;
      background-color: ivory;
    }
    body {
      margin: 0;
    }
    .content {
      padding: 20px;
    }
    .card {
      background-color: white;
      box-shadow: 2px 2px 12px 1px rgba(140, 140, 140, .5);
    }
    .cards {
      max-width: 700px;
      margin: 0 auto;
      display: grid;
      grid-gap: 2rem;
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    }
    .reading {
      font-size: 2.8rem;
    }
    .packet {
      color: #bebebe;
    }
    .condition {
      color: black;
    }
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h1 font-size: 1.7rem;>Air Care WEB SERVER</h1>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); }
  else { xhr.open("GET", "/update?state=0", true); }
  xhr.send();
}

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var inputChecked;
      var outputStateM;
      if( this.responseText == 1){ 
        inputChecked = true;
        outputStateM = "On";
      }
      else { 
        inputChecked = false;
        outputStateM = "Off";
      }
      document.getElementById("output").checked = inputChecked;
      document.getElementById("outputState").innerHTML = outputStateM;
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}, 1000 ) ;
</script>
<script>
    if (!!window.EventSource) {
      var source = new EventSource('/events');

      source.addEventListener('open', function (e) {
        console.log("Events Connected");
      }, false);
      source.addEventListener('error', function (e) {
        if (e.target.readyState != EventSource.OPEN) {
          console.log("Events Disconnected");
        }
      }, false);
    }
  </script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    String outputStateValue = outputState();
    buttons+= "<label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}

String outputState(){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
  return "";
}

void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

void setup() {
  strip.begin(); //네오픽셀을 초기화하기 위해 모든LED를 off시킨다
  strip.show();

  pinMode(buttonPin, INPUT);
  pinMode(27, OUTPUT);
  pinMode(output, OUTPUT);
  digitalWrite(output, LOW);
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);

  // WiFi 환경 설정.
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  
  // Server Events
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });

  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      digitalWrite(output, inputMessage.toInt());
      ledState = !ledState;
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  // Send a GET request to <ESP_IP>/state
  server.on("/state", HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(digitalRead(output)).c_str());
  });
  
  server.addHandler(&events);
  server.begin();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    if(ledState==1) {
      get_weather();
      Serial.printf("pm10 = %.2f \n", microdust_pm10);
      events.send(String(microdust_pm10).c_str(),"microdust_pm10",millis());
      Serial.println();
    }
    else {
      digitalWrite(27, LOW);
      colorWipe(strip.Color(0,0,0), 0);
    }
    Serial.printf("ledState : %d", ledState);
    events.send("ping",NULL,millis());
    Serial.println();
    lastTime = millis();
    delay(30000);
  }

  int reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH) {
        ledState = !ledState;
      }
    }
  }
  lastButtonState = reading;
}
