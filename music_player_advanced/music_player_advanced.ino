#include <SPI.h>
#include <ESP8266WiFi.h>

// intro to Superman theme [120, 0, 17, 4, 11, 12, 11, 12, 4, 12, 11, 2, 11, 4, 0b00010100, 4, 11, 2, 4, 4, 11, 12, 11, 12, 4, 12, 11, 2, 11, 16, 0b00011000, 8, 0, 16, 0b00010110, 2, 0b00010110]

const char ssid[] = "<SSID>";
const char pass[] = "<PASSWORD>";
const int LED = 13;
const int SPEAKER = 15;

const int SCALE[] = {440, 466, 494, 523, 554, 587, 622, 659, 698, 739, 784, 830};

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

boolean tryCommand(WiFiClient client){
  int bpm = 1000 / (((B0 << 16) | client.read())/60) * 4 ;
  int len = (client.read() << 8) | (client.read());

  Serial.print("Whole note length: ");
  Serial.println(bpm);
  
  if(len > 0){
    delete [] notes;

    notes = new NOTE[len];

    for(int x = 0; x < len; x++){      
      byte note_len_b = client.read();
      byte n_len = B00111111 & note_len_b;
      byte n_len_multi = B00000011 & (note_len_b >> 6);

//      Serial.println(n_len);
//      Serial.println(n_len_multi);

      byte note_b = client.read();
      byte note_index = B00001111 & note_b;
      byte note_shift = B00000111 & (note_b >> 4);
      byte note_shift_n = B00000001 & (note_b >> 7);

//      Serial.println(note_index);
//      Serial.println(note_shift);
//      Serial.println(note_shift_n);

      NOTE note;

      if(note_index == 0){
        note.freq = 0;
      }
      else{
        note.freq = SCALE[note_index - 1] * (note_shift * (note_shift_n == 0 ? 1 : -1) + 1);
      }

      note.interval = ((n_len_multi + 1) * bpm)/n_len;

//      Serial.print("Note: ");
//      Serial.println(note.freq);
//      Serial.print("Interval: ");
//      Serial.println(note.interval);

      notes[x] = note;
    }

    currentIndex = 0;
    numNotes = len;
    return true;
  }

  return false;
}

void checkIncomingRequest(){
  WiFiClient client = server.available();
  if(client)
  {
    int total = 0;
    Serial.println("[Client connected]");
    while(client.connected())
    {
      if(client.available())
      {
        String line = client.readStringUntil('\r');
        Serial.print(line);
        total += line.length();
        
        if(line.length() == 1 && line[0] == '\n')
        {
          client.read(); //increment pointer by one
          if(tryCommand(client)){
            client.println(buildResponse("Playing"));
          }
          else{
            client.println(buildResponse("Invalid song data"));
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
      if(note.freq == 0){
        noTone(SPEAKER);
      }
      else {
        tone(SPEAKER, note.freq);
      }
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
    if(currentTimeLeft < 50) {
      noTone(SPEAKER); // Create subtle break between notes
    }
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
