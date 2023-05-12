#define FINGER_RXD 47 // GREEN
#define FINGER_TXD 48 // WHITE

#define FINGER_ID 1

#include <Adafruit_Fingerprint.h>

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);

void init_fingerprint(){
  Serial1.begin(57600, SERIAL_8N1, FINGER_RXD, FINGER_TXD);
  finger.begin(57600);

  //finger.setSecurityLevel(FINGERPRINT_SECURITY_LEVEL_1);

  if(finger.verifyPassword()){
    Serial.println("Found fingerprint sensor!");
    Serial.println(F("Reading sensor parameters..."));
    finger.getParameters();
    Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
    Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
    Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
    Serial.print(F("Security level: ")); Serial.println(finger.security_level);
    Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
    Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
    Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);
    finger.getTemplateCount();
    Serial.print(F("Fingerprint: ")); Serial.println(finger.templateCount);
  }else{
    Serial.println("Did not find fingerprint sensor");
    return;
  }
}

void delete_fingerprints(){
  finger.emptyDatabase();
}

int process_fingerprint(bool enroll){
  int p = -1;
  int image_id = 0;
  do{
    image_id++;
    while (p != FINGERPRINT_OK) {
      p = finger.getImage();
      switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
      }
    }
    
    p = finger.image2Tz(image_id);
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image converted");
        break;
      case FINGERPRINT_IMAGEMESS:
        Serial.println("Image too messy");
        return -1;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return -1;
      case FINGERPRINT_FEATUREFAIL:
        Serial.println("Could not find fingerprint features");
        return -1;
      case FINGERPRINT_INVALIDIMAGE:
        Serial.println("Could not find fingerprint features");
        return -1;
      default:
        Serial.println("Unknown error");
        return -1;
    }
    
    delay(1000);
    p = 0;
    while (p != FINGERPRINT_NOFINGER) {
      p = finger.getImage();
    }
  }while(enroll && image_id <= 1);

  if(enroll){
    p = finger.createModel();
    if (p == FINGERPRINT_OK) {
      Serial.println("Prints matched!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Communication error");
      return -1;
    } else if (p == FINGERPRINT_ENROLLMISMATCH) {
      Serial.println("Fingerprints did not match");
      return -1;
    } else {
      Serial.println("Unknown error");
      return -1;
    }
  
    Serial.print("ID "); Serial.println(FINGER_ID);
    p = finger.storeModel(FINGER_ID);
    if (p == FINGERPRINT_OK) {
      Serial.println("Stored!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Communication error");
      return -1;
    } else if (p == FINGERPRINT_BADLOCATION) {
      Serial.println("Could not store in that location");
      return -1;
    } else if (p == FINGERPRINT_FLASHERR) {
      Serial.println("Error writing to flash");
      return -1;
    } else {
      Serial.println("Unknown error");
      return -1;
    }
    return FINGER_ID;
  }else{
    p = finger.fingerSearch();
    if (p == FINGERPRINT_OK) {
      Serial.println("Found a print match!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Communication error");
      return -1;
    } else if (p == FINGERPRINT_NOTFOUND) {
      Serial.println("Did not find a match");
      return -1;
    } else {
      Serial.println("Unknown error");
      return -1;
    }
    Serial.print("Found ID #"); Serial.print(finger.fingerID);
    Serial.print(" with confidence of "); Serial.println(finger.confidence);
    return finger.fingerID;
  }
}

int check_finger(){
  return process_fingerprint(false);
}

int enroll_finger(){
  return process_fingerprint(true);
}
