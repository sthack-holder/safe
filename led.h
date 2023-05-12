// VCC -> LITTLE DASH

#define ORANGE_LED 42 // CROSS
#define RED_LED 2 // BIG DASH
#define GREEN_LED 1 // DOT

void init_led(){
  pinMode(ORANGE_LED, INPUT);
  pinMode(RED_LED, INPUT);
  pinMode(GREEN_LED, INPUT);
}

void led_on(int led_id){
  pinMode(led_id, OUTPUT);
  digitalWrite(led_id, LOW);
}

void led_off(int led_id){
  digitalWrite(led_id, HIGH);
  pinMode(led_id, INPUT);
}

void led_blink(int led_id, int duration){
  led_on(led_id);
  delay(duration);
  led_off(led_id);
}
