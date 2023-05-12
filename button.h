// GND -> WHITE

#define EXT_BUTTON 41 // LOOONNNNG LINES

#define GREEN_BUTTON 45
#define RED_BUTTON 0

void init_button(){
  pinMode(EXT_BUTTON, INPUT_PULLDOWN);
  pinMode(GREEN_BUTTON, INPUT_PULLUP);
  pinMode(RED_BUTTON, INPUT_PULLUP);
}

bool is_button_press(){
  return digitalRead(EXT_BUTTON);
}

bool is_green_button_press(){
  return !digitalRead(GREEN_BUTTON);
}

bool is_red_button_press(){
  return !digitalRead(RED_BUTTON);
}
