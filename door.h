// GND -> RED

#define OPEN_SENSOR 19 // YELLOW
#define CLOSE_SENSOR 20 // BROWN
#define KEY_SENSOR 21 // ORANGE

#define CLOSE_MOTOR 3 // INT 1
#define OPEN_MOTOR 46 // INT 2

void init_door(){
  pinMode(OPEN_SENSOR, INPUT_PULLUP);
  pinMode(CLOSE_SENSOR, INPUT_PULLUP);
  pinMode(KEY_SENSOR, INPUT_PULLUP);
  
  pinMode(OPEN_MOTOR, OUTPUT);
  pinMode(CLOSE_MOTOR, OUTPUT);
}

bool is_door_opened(){
  return !digitalRead(OPEN_SENSOR);
}

bool is_door_closed(){
  return !digitalRead(CLOSE_SENSOR);
}

bool is_key_used(){
  return digitalRead(KEY_SENSOR);
}

void open_door(){
  digitalWrite(OPEN_MOTOR, HIGH);
  while(!is_door_opened()){
    delay(100);
  }
  digitalWrite(OPEN_MOTOR, LOW);
}

void close_door(){
  digitalWrite(CLOSE_MOTOR, HIGH);
  while(!is_door_closed()){
    delay(100);
  }
  digitalWrite(CLOSE_MOTOR, LOW);
}

void toggle_door(){
  if(is_door_opened()){
    close_door();
  }else{
    open_door();
  }
}
