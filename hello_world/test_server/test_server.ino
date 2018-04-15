#include <SPI.h>
#include <ESP8266WiFi.h>

char ssid[] = "IKnowWhatYouDidLastSummer";
char pass[] = "RollyIsAw3som3";
int status = WL_IDLE_STATUS;
const int BUFFER_SIZE = 1024;
const int LED = 13;
const int SPEAKER = 15;

WiFiServer server(80);

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(SPEAKER, OUTPUT);
  digitalWrite(LED, HIGH);
  tone(SPEAKER, 440);
  
  Serial.begin(9600);
  Serial.println("Attempting to connect to network...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println(".");
  }

  server.begin();
  Serial.print("Connected to wifi. My address: ");
  IPAddress myAddress = WiFi.localIP();
  Serial.println(myAddress);
}

String BuildResponse(String stat){
  String html = String("<!DOCTYPE HTML>") +
                "<html>" +
                "Turning LED: " + stat +
                "</html>"+
                "\r\n";
  
  return String("HTTP/1.1 200 OK\r\n") +
                "Content-Type: text/html\r\n" +
                "Content-Length: " + html.length() + "\r\n" +
                "\r\n" + html;
                
}

boolean tryCommand(String command){
  if(command == "on"){
    digitalWrite(LED, HIGH);
    return true;
  }
  else if(command == "off") {
    digitalWrite(LED, LOW);
    return true;
  }

  return false;
}

void loop() {
  if ( WiFi.status() != WL_CONNECTED){
    return;
  }
  
  WiFiClient client = server.available();
  if(client)
  {
    String command = "";
    Serial.println("[Client connected]");
    while(client.connected())
    {
      if(client.available())
      {
        String line = client.readStringUntil('\r');
        Serial.print(line);

        if(line.startsWith("GET /")){
          command = line.substring(5, line.indexOf(' ', 5));
        }
        else if(line.length() == 1 && line[0] == '\n')
        {
          if(command != "" && tryCommand(command)){
            int currentState = digitalRead(13);
            String state = currentState == HIGH ? "ON" : "OFF";
            client.println(BuildResponse(state));
          }
          else {
            int currentState = digitalRead(13);
            String state = String("UNCHANGED (") + (currentState == HIGH ? "ON" : "OFF") + ")";
            client.println(BuildResponse(state));
          }
          break;
        }
      }
    }

    delay(1);

    client.stop();
    Serial.println("[Client disconnected]");
  }
}
