#include <SPI.h>
#include <ESP8266WiFi.h>

const char ssid[] = "IKnowWhatYouDidLastSummer";
const char pass[] = "RollyIsAw3som3";
const int BUFFER_SIZE = 1024;
const int LED = 13;
const int SPEAKER = 15;

const int SCALE[] = {440, 493, 523, 587, 659, 698, 783};
const byte LOWER_A = 65;
const byte UPPER_A = 97;
const byte ZERO = 48;

WiFiServer server(80);

struct NOTE {
  int freq;
  int interval;
};

NOTE* notes;
int numNotes = 0;
int currentIndex = -1;
int lastCheckTime = 0;
int currentTimeLeft = 0;

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(SPEAKER, OUTPUT);
  
  digitalWrite(LED, HIGH);  // Turn on LED
  noTone(SPEAKER); // Turn off Tone
  
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

String buildResponse(String stat){
  String html = String("<!DOCTYPE HTML>") +
                "<html>" +
                "Status: " + stat +
                "</html>"+
                "\r\n";
  
  return String("HTTP/1.1 200 OK\r\n") +
                "Content-Type: text/html\r\n" +
                "Content-Length: " + html.length() + "\r\n" +
                "\r\n" + html;
                
}

int tryCommand(String command){
  // if not at least one note with interval
  // then retrun immediately since it's an
  // invalid command
  if(command.length() < 2)
  {
    return 0;
  }
  
  currentIndex = -1;
  delete [] notes;

  // assign new array based on size of command
  notes = new NOTE[command.length() / 2];

  for(int x = 0; x < command.length(); x += 2){
    // Get index for note pitch
    byte index = command[x] - LOWER_A;
    if(index < 0 || index > 6){
      index = command[x] - UPPER_A;
    }
    Serial.print("Got note index: ");
    Serial.println(index);

    // if valid note then add a new note to the notes array
    if(index >= 0 && index <= 6)
    {
      byte interval = command[x + 1] - ZERO;

      if(interval > 0){
        Serial.print("Got note interval: ");
        Serial.println(interval);
        NOTE note;
        note.freq = SCALE[index];
        note.interval = interval * 125;
        notes[x/2] = note;
      }
      else{
        delete [] notes;
        return x/2;
      }
    }
    // else delete the notes array and return the index that failed
    else{
      delete [] notes;
      return x/2;
    }
  }

  Serial.print("Total number of notes: ");
  Serial.println(command.length()/2);
  numNotes = command.length()/2;
  currentIndex = 0;
  return -1;
}

void checkIncomingRequest(){
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
          if(command.length() > 0)
          {
            int result = tryCommand(command);
            if(result < 0){
              client.println(buildResponse("Playing"));
            }
            else{
              client.println(buildResponse(String("Invalid song. Problem at note ") + result));
            }
          }
          else{
            client.println(buildResponse("No/Invalid command"));
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

void handlePlaySong(){
  if(currentTimeLeft <= 0)
  {
    if(currentIndex < numNotes){
      NOTE note = notes[currentIndex];
      tone(SPEAKER, note.freq);
      currentTimeLeft = note.interval;
      currentIndex++;
      lastCheckTime = millis();
    }
    else{
      currentIndex = -1;
      noTone(SPEAKER);
    }
  }
  else{
    currentTimeLeft -= millis() - lastCheckTime;
    lastCheckTime = millis();
  }
}

void loop() {
  if ( WiFi.status() != WL_CONNECTED){
    return;
  }
  
  checkIncomingRequest();

  if(currentIndex > -1){
    handlePlaySong();
  }
}
