#include <Wire.h>

void setup(){
  pinMode(2, INPUT);
  Wire.begin(0x0A);
  Wire.onReceive(receiveEvent); 
  Wire.onRequest(requestEvent);
  Serial.begin(9600); // 
  Serial.println("Init");
}

int command;
int answer;
int state = 0;
int arduino_state;
int mode, temp, light;
char button_yes [] = "YES";
char button_no [] = "NO";

void loop(){
  char rs [50];

  if (state == 0){ 
    if (mode == 1){
      sprintf(rs, "<msg><m>%d</m><t>%d</t></msg>", &mode, &temp);
      Serial.println(rs);
      state = 1;
    } 
    else if (mode == 2){
      sprintf(rs, "<msg><m>%d</m><l>%d</l></msg>", &mode, &light);
      Serial.println(rs);
      state = 1;
    } 
    else if (mode == 3){
      sprintf(rs, "<msg><m>%d</m><l>%d</l><t>%d</t></msg>", mode, light, temp);
      Serial.println(rs);
      state = 1;
    }
  }
}

void check_state(){
  if (digitalRead(2) == 1){
    Serial.println(button_yes);
    arduino_state = 1;
  }
  else {
    Serial.println(button_no);
    arduino_state = 0;
  }
}

void receiveEvent(int howMany){
  int temp_aux, light_aux;
  Serial.println("receiveEvent");
  while(Wire.available()){
    mode = Wire.read();
    if (mode == 1){
      temp_aux = Wire.read();
      temp = (temp_aux << 8) + Wire.read();
    }
    else if (mode == 2){
      light_aux = Wire.read();
      light = (light_aux << 8) + Wire.read();
    }
    else if (mode == 3){
      temp_aux = Wire.read();
      temp = (temp_aux << 8) + Wire.read();
      light_aux = Wire.read();
      light = (light_aux << 8) + Wire.read();
    }
  }
}

void requestEvent(){
  Serial.println("Sending reply");
  check_state();
  Wire.write(arduino_state);
  state = 0;
  mode = 0;
}
