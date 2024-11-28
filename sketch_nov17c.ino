#include <WiFi.h>
#include <WiFiClientSecure.h>
// Comentário de última hora: 28/11
// O código irá iniciar o Web Server caso tudo tiver ok com as credenciais e vai printar o IP
// O API do PushBullet irá mostrar no Serial se está funcionando ou não, está no Serial Monitor se haver erro de comunicação
// Haverá uma constante atualização pelo JavaScript toda vez que haver uma mudança de estado do LED ou Sensor Magnético
// Sempre enviará a cada 10 segundos uma notificação PUSH pelo PushBullet caso estiver com a condições corretas
// Condições para enviar mensagem: Led X Ligado e Sensor Y Aberto
const char *ssid = "Brayan";         
const char *password = "12345678";   


const char *host = "api.pushbullet.com";
const int httpsPort = 443; 
const char *accessToken = "o.p6xGrUSTf4q1GdPFKxf800BWUX44hopV"; 


const int ledPin1 = 15;    
const int ledPin2 = 2;    
const int ledPin3 = 4;    
const int sensorPin1 = 23; 
const int sensorPin2 = 22; 
const int sensorPin3 = 18; 
const int buzzerPin = 5;   

bool ledState1 = false;
bool ledState2 = false;
bool ledState3 = false;

WiFiServer server(80); 

unsigned long lastNotificationTime = 0; 

void setup() {
  Serial.begin(115200);


  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);
  pinMode(sensorPin1, INPUT);
  pinMode(sensorPin2, INPUT);
  pinMode(sensorPin3, INPUT);
  pinMode(buzzerPin, OUTPUT);

  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado.");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());

  server.begin(); 
}

void loop() {
  
  checkSensors();

  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
  }
}

void checkSensors() {
  static bool lastSensorState1 = false;
  static bool lastSensorState2 = false;
  static bool lastSensorState3 = false;

  bool sensor1State = digitalRead(sensorPin1);
  bool sensor2State = digitalRead(sensorPin2);
  bool sensor3State = digitalRead(sensorPin3);

  if (sensor1State != lastSensorState1) {
    if (sensor1State && ledState1) {
      if (millis() - lastNotificationTime > 10000) { 
        sendPushNotification("Alerta de Sensor", "Sensor Porta Aberto: Alerta!");
        lastNotificationTime = millis(); 
      }
      digitalWrite(buzzerPin, HIGH);
    } else {
      digitalWrite(buzzerPin, LOW); 
    }
    lastSensorState1 = sensor1State;
  }

  if (sensor2State != lastSensorState2) {
    if (sensor2State && ledState2) {
      if (millis() - lastNotificationTime > 10000) {
        sendPushNotification("Alerta de Sensor", "Sensor Janela Esquerda Aberto: Alerta!");
        lastNotificationTime = millis(); 
      }
     
      digitalWrite(buzzerPin, HIGH);
    } else {
      digitalWrite(buzzerPin, LOW); 
    }
    lastSensorState2 = sensor2State;
  }

 
  if (sensor3State != lastSensorState3) {
    if (sensor3State && ledState3) {
      if (millis() - lastNotificationTime > 10000) { 
        sendPushNotification("Alerta de Sensor", "Sensor Janela Direita Aberto: Alerta!");
        lastNotificationTime = millis(); 
      }
      
      digitalWrite(buzzerPin, HIGH);
    } else {
      digitalWrite(buzzerPin, LOW); 
    }
    lastSensorState3 = sensor3State;
  }
}

void handleClient(WiFiClient client) {
  Serial.println("Novo Cliente.");
  String currentLine = "";
  bool isAPIRequest = false;

  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      Serial.write(c);

      if (c == '\n') {
        if (currentLine.length() == 0) {
          if (isAPIRequest) {
            sendSensorStatus(client);
          } else {
            sendHTMLPage(client);
          }
          break;
        } else {
          currentLine = "";
        }
      } else if (c != '\r') {
        currentLine += c;
        if (currentLine.endsWith("GET /sensors")) {
          isAPIRequest = true;
        }
      }

      
      if (currentLine.endsWith("GET /T1")) {
        ledState1 = !ledState1;
        digitalWrite(ledPin1, ledState1 ? HIGH : LOW);
      }
      if (currentLine.endsWith("GET /T2")) {
        ledState2 = !ledState2;
        digitalWrite(ledPin2, ledState2 ? HIGH : LOW);
      }
      if (currentLine.endsWith("GET /T3")) {
        ledState3 = !ledState3;
        digitalWrite(ledPin3, ledState3 ? HIGH : LOW);
      }
    }
  }
  client.stop();
  Serial.println("Cliente desconectado.");
  delay(100);
}

void sendPushNotification(const char *title, const char *body) {
  WiFiClientSecure client;
  client.setInsecure();

  if (!client.connect(host, httpsPort)) {
    Serial.println("Falha ao conectar no Pushbullet!");
    return;
  }

  String url = "/v2/pushes";
  String message = "{\"type\": \"note\", \"title\": \"" + String(title) + "\", \"body\": \"" + String(body) + "\"}";

  client.println("POST " + url + " HTTP/1.1");
  client.println("Host: " + String(host));
  client.println("Authorization: Bearer " + String(accessToken));
  client.println("Content-Type: application/json");
  client.println("Content-Length: " + String(message.length()));
  client.println();
  client.println(message);

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }
  Serial.println("Notificação enviada!");
}

void sendSensorStatus(WiFiClient client) {
  String response = "{\"sensors\": {";
  response += "\"sensor1\": \"" + getSensorStatus(ledState1, sensorPin1) + "\","; 
  response += "\"sensor2\": \"" + getSensorStatus(ledState2, sensorPin2) + "\","; 
  response += "\"sensor3\": \"" + getSensorStatus(ledState3, sensorPin3) + "\"";
  response += "}}";

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println();
  client.print(response);
}

void sendHTMLPage(WiFiClient client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  client.print("<html><head><style>");
  client.print("body {background-color: #800080; font-family: Arial, sans-serif; text-align: center;} ");
  client.print("h1 {color: white;} ");
  client.print(".container {border: 2px solid white; display: inline-block; padding: 20px;} ");
  client.print("p {font-size: 18px; color: white;} ");
  client.print("a {text-decoration: none; font-size: 20px; background-color: #4B0082; color: white; padding: 10px 20px; border-radius: 5px;} ");
  client.print("a:hover {background-color: #8A2BE2;} ");
  client.print("</style>");
  client.print("<script>");
  client.print("setInterval(function() {");
  client.print("  fetch('/sensors') ");
  client.print("    .then(response => response.json())");
  client.print("    .then(data => {");
  client.print("      document.getElementById('sensor1Status').innerText = data.sensors.sensor1;"); 
  client.print("      document.getElementById('sensor2Status').innerText = data.sensors.sensor2;");
  client.print("      document.getElementById('sensor3Status').innerText = data.sensors.sensor3;");
  client.print("    });");
  client.print("}, 2000);");
  client.print("</script>");
  client.print("</head><body>");
  client.print("<h1>Sistema de Alarme</h1>");
  client.print("<div class='container'>");

  
  client.print("<p><a href='/T1'>Controle LED 1</a><br>");
  client.print("<span id='sensor1Status'>" + getSensorStatus(ledState1, sensorPin1) + "</span></p>");
  client.print("<p><a href='/T2'>Controle LED 2</a><br>");
  client.print("<span id='sensor2Status'>" + getSensorStatus(ledState2, sensorPin2) + "</span></p>");
  client.print("<p><a href='/T3'>Controle LED 3</a><br>");
  client.print("<span id='sensor3Status'>" + getSensorStatus(ledState3, sensorPin3) + "</span></p>");
  
  client.print("</div>");
  client.print("</body></html>");
}

String getSensorStatus(bool ledState, int sensorPin) {
  if (!ledState) {
    return "Sensor Desativado";
  } else {
    return (digitalRead(sensorPin)) ? "Aberto - Alerta!" : "Fechado - Tudo ok";
  }
}
