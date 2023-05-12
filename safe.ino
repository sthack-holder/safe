#define USE_FACE
//#define USE_FINGERPRINT

#include "button.h"
#include "led.h"
#include "door.h"
#include "sd.h"
#include "spiffs.h"
#if defined(USE_FACE)
#include "camera.h"
#endif
#if defined(USE_FINGERPRINT)
#include "fingerprint.h"
#endif

void setup() {
  Serial.begin(115200);
  init_button();
  init_led();
  init_door();
  init_sd();
  init_spiffs();
  #if defined(USE_FACE)
  init_camera();
  #endif
  #if defined(USE_FINGERPRINT)
  init_fingerprint();
  #endif

  #if defined(USE_FACE)
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file){
    String filename = String(file.name());
    if(filename.startsWith("face-") && filename.endsWith(".jpg")){
      Serial.println("Enroll: /"+filename);
      enroll_from_file(file);
    }
    file = root.openNextFile();
  }
  #endif
}

void loop() {
  handle_reset();
  handle_action();
}

void handle_action(){
  if(!is_button_press()){return;}
  unsigned long start_time = millis();
  while(is_button_press()){
    delay(500);
    if((millis() - start_time) >= (3000)){
      if(is_door_opened()){
        enroll_process();
      }
      return;
    }
  }
  if(is_door_opened()){close_door();return;}
  unlock_process();
}

void unlock_process(){
  led_blink(GREEN_LED, 250);
  #if defined(USE_FACE)
  if(check_face() <= 0){led_blink(RED_LED, 250);return;}
  led_blink(GREEN_LED, 250);
  #endif
  #if defined(USE_FINGERPRINT)
  if(check_finger() <= 0){led_blink(RED_LED, 250);return;}
  led_on(GREEN_LED);
  #endif
  while(!is_button_press()){delay(100);}
  led_off(GREEN_LED);
  while(is_button_press()){delay(100);}
  open_door();
}

void enroll_process(){
  led_on(ORANGE_LED);
  
  delay(1000);

  #if defined(USE_FACE)
  if(enroll_face() > 0){
    led_blink(GREEN_LED, 1000);
  }else{
    led_blink(RED_LED, 1000);
    led_off(ORANGE_LED);
    return;
  }
  #endif

  #if defined(USE_FINGERPRINT)
  if(enroll_finger()){
    led_blink(GREEN_LED, 1000);
  }else{
    led_blink(RED_LED, 1000);
    led_off(ORANGE_LED);
    return;
  }
  #endif
  
  led_off(ORANGE_LED);
}

void handle_reset(){
  if(!is_door_opened()){return;}
  unsigned long start_time = millis();
  while(is_red_button_press()){
    delay(500);
    if((millis() - start_time) >= (5000)){
      #if defined(USE_FACE)
      delete_faces();
      #endif
      #if defined(USE_FINGERPRINT)
      delete_fingerprints();
      #endif
      led_blink(RED_LED, 1000);
      open_door();
      return;
    }
  }
}
