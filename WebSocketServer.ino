#include <SPI.h>
#include <Ethernet.h>

// Enabe debug tracing to Serial port.
#define DEBUG

// Here we define a maximum framelength to 64 bytes. Default is 256.
#define MAX_FRAME_LENGTH 64

#include <WebSocket.h>

byte mac[] = { 0x52, 0x4F, 0x43, 0x4B, 0x45, 0x54 };
byte ip[] = { 192, 168, 1 , 10 };

const int voiceSensor = A0;
const int shockSensor = 3; 
const int led = 13;
unsigned long last_time;
bool led_flag = 0;

const int max_voice = 1000;
const int min_voice = 300;

const int max_size = 20;
float values[max_size];
int i = 0;

void add(float new_val){
  if (i>max_size-1) i=0;
  values[i] = new_val;
  i++;
}

float mid(){
  float result = 0;
  for(int j=0;j<max_size;j++){
    result+=values[j];
  }
  return result/(float)max_size;
}

float get_voice(){
  float var = mid();
  if ((analogRead(voiceSensor)>min_voice)&&(analogRead(voiceSensor)<max_voice)) 
  {
    add(analogRead(voiceSensor));
    if (abs(var - analogRead(voiceSensor))>20){
      led_flag = 1;
      last_time = millis();
      digitalWrite(led, HIGH);
    }
    else {
      if ((millis() - last_time > 1000) && led_flag)
      {
         digitalWrite(led, LOW);
         led_flag = 0;
      }
    }
  }
  return var;
}

int get_shock(){
  return digitalRead(shockSensor);
}

// Create a Websocket server
WebSocketServer wsServer;

void onConnect(WebSocket &socket) {
  Serial.println("onConnect called");
}


// You must have at least one function with the following signature.
// It will be called by the server when a data frame is received.
void onData(WebSocket &socket, char* dataString, byte frameLength) {
  
#ifdef DEBUG
  Serial.print("Got data: ");
  Serial.write((unsigned char*)dataString, frameLength);
  Serial.println();
#endif
  
  // Just echo back data for fun.
  socket.send(dataString, strlen(dataString));
}

void onDisconnect(WebSocket &socket) {
  Serial.println("onDisconnect called");
}

void setup() {
#ifdef DEBUG  
  Serial.begin(19200);
#endif
  Ethernet.begin(mac, ip);
  wsServer.registerConnectCallback(&onConnect);
  wsServer.registerDataCallback(&onData);
  wsServer.registerDisconnectCallback(&onDisconnect);  
  wsServer.begin();

  pinMode(voiceSensor, INPUT);
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  
  delay(10); // Give Ethernet time to get ready
}

void loop() {
  wsServer.listen();
  float sound_sensor_val = get_voice();
  int sound_shock_val = get_shock();
  
  char str_voice_val[5] = {' '} ;
  char str_shock_val[1];
  char response[6];
  
  int val_int = (int)sound_sensor_val;
  float val_float = (abs(sound_sensor_val) - abs(val_int)) * 10000;
  int val_fra = (int)val_float;
  sprintf (str_voice_val, "%d.%d", val_int, val_fra);

    
  if (sound_shock_val == 1){
    str_shock_val[0] = '1';
  }
  else{
    str_shock_val[0] = '0';
  }

  response[0]= str_shock_val[0];
  for(int i=1; i<7;i++){
    response[i] = str_voice_val[i-1];
  }

  
  Serial.println(response);
  delay(10);
  
  if (wsServer.connectionCount() > 0) {
    //wsServer.send("Hello to Aleksey Vladimirovich!!!", 34);
    wsServer.send(response, 7);
    Serial.println("successfully sent");
  }
  else{
    Serial.println("NO CONNECTION WITH CLIENT");
  }
}
