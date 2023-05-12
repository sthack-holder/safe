#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
#include "camera_pins.h"

#include "esp_camera.h"

#include "img_converters.h"
#include "fb_gfx.h"
#include "driver/ledc.h"
#include "sdkconfig.h"

#include <vector>
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"

#include "face_recognition_tool.hpp"
#include "face_recognition_112_v1_s16.hpp"
#include "face_recognition_112_v1_s8.hpp"

#define MAX_FACE_STORAGE 5
#define RETRY_COUNT 3

FaceRecognition112V1S8 recognizer;

void init_camera(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  //config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if(config.pixel_format == PIXFORMAT_JPEG){
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

  s->set_vflip(s, 1);
}

void delete_faces(){
  recognizer.clear_id(true);
}


static int run_face_recognition(fb_data_t *fb, std::list<dl::detect::result_t> *results, bool enroll, bool flash_storage){
  
  std::vector<int> landmarks = results->front().keypoint;

  Serial.print("landmarks : ");
  for(int i : landmarks){
    Serial.print(i);
    Serial.print(",");
  }
  Serial.println();

  //Enroll: /demo.jpg
  //landmarks : 138,100,142,151,161,130,184,100,179,151,
  //Enroll: /face-owner.jpg
  //landmarks : 131,80,135,119,149,106,171,80,169,118,
  
  int id = -1;

  Tensor<uint8_t> tensor;
  tensor.set_element((uint8_t *)fb->data).set_shape({fb->height, fb->width, 3}).set_auto_free(false);
  int enrolled_count = recognizer.get_enrolled_id_num();
  
  Serial.print("Enrolled count: ");
  Serial.println(enrolled_count, HEX);

  if (enrolled_count < MAX_FACE_STORAGE && enroll){
      id = recognizer.enroll_id(tensor, landmarks, "", flash_storage);
      Serial.print("Enrolled ID: ");
      Serial.println(id);
      //rgb_printf(fb, FACE_COLOR_CYAN, "ID[%u]", id);
  }

  face_info_t recognize = recognizer.recognize(tensor, landmarks);
  if(recognize.id >= 0){
      //rgb_printf(fb, FACE_COLOR_GREEN, "ID[%u]: %.2f", recognize.id, recognize.similarity);
      Serial.print("Identified : ");
      Serial.print(recognize.id);
      Serial.print(" -> ");
      Serial.print(recognize.similarity);
      Serial.println("%");
  } else {
      //rgb_print(fb, FACE_COLOR_RED, "Intruder Alert!");
      Serial.println("Intruder");
  }
  return recognize.id;
}

int enroll_from_file(File file){
  int face_id = 0;
  
  uint8_t *buf;
  int buffer_size = sizeof(uint8_t)*file.size();
  buf = (uint8_t*) malloc (buffer_size);
  
  long i = 0;
  while (file.available()) {
    buf[i] = file.read();
    i++;  
  }
  
  size_t out_len, out_width, out_height;
  uint8_t *out_buf;
  bool s;
  
  out_width = 320;
  out_height = 240;
  out_len = out_width * out_height * 3;
  out_buf = (uint8_t*)malloc(out_len);
  if (!out_buf) {
      Serial.println("out_buf malloc failed");
      return 0;
  }
  s = fmt2rgb888(buf, buffer_size, PIXFORMAT_JPEG, out_buf);
  if (!s) {
      free(out_buf);
      Serial.println("to rgb888 failed");
      return 0;
  }

  fb_data_t rfb;
  rfb.width = out_width;
  rfb.height = out_height;
  rfb.data = out_buf;
  rfb.bytes_per_pixel = 3;
  rfb.format = FB_BGR888;

  // Detect human faces
  HumanFaceDetectMSR01 s1(0.1F, 0.5F, 10, 0.2F);
  HumanFaceDetectMNP01 s2(0.5F, 0.3F, 5);
  std::list<dl::detect::result_t> &candidates = s1.infer((uint8_t *)out_buf, {(int)out_height, (int)out_width, 3});
  std::list<dl::detect::result_t> &results = s2.infer((uint8_t *)out_buf, {(int)out_height, (int)out_width, 3}, candidates);

  if(results.size() > 0){
    face_id = run_face_recognition(&rfb, &results, true, false);
  }
  free(out_buf);
  return face_id;
}

int process_image(bool enroll){
  int face_id = 0;
  
  camera_fb_t *fb = NULL;

  fb = esp_camera_fb_get();

  size_t out_len, out_width, out_height;
  uint8_t *out_buf;
  bool s;

  out_len = fb->width * fb->height * 3;
  out_width = fb->width;
  out_height = fb->height;
  out_buf = (uint8_t*)malloc(out_len);
  if (!out_buf) {
      Serial.println("out_buf malloc failed");
      return 0;
  }
  Serial.println("FORMAT : ");
  Serial.println(fb->format);
  s = fmt2rgb888(fb->buf, fb->len, fb->format, out_buf);
  
  esp_camera_fb_return(fb);
  if (!s) {
      free(out_buf);
      Serial.println("to rgb888 failed");
      return 0;
  }

  fb_data_t rfb;
  rfb.width = out_width;
  rfb.height = out_height;
  rfb.data = out_buf;
  rfb.bytes_per_pixel = 3;
  rfb.format = FB_BGR888;

  // Detect human faces
  HumanFaceDetectMSR01 s1(0.1F, 0.5F, 10, 0.2F);
  HumanFaceDetectMNP01 s2(0.5F, 0.3F, 5);
  std::list<dl::detect::result_t> &candidates = s1.infer((uint8_t *)out_buf, {(int)out_height, (int)out_width, 3});
  std::list<dl::detect::result_t> &results = s2.infer((uint8_t *)out_buf, {(int)out_height, (int)out_width, 3}, candidates);
  
  Serial.print("Faces: ");
  Serial.println(results.size());
  if(results.size() > 0){
    face_id = run_face_recognition(&rfb, &results, enroll, true);
    //draw_face_boxes(&rfb, &results, face_id);
  }
  
  //s = fmt2jpg_cb(out_buf, out_len, out_width, out_height, PIXFORMAT_RGB888, 90, jpg_encode_stream, &jchunk);
  free(out_buf);

  return face_id;
}

int try_process_image(bool enroll){
  int result = 0;
  int retry = 0;
  do{
    result = process_image(enroll);
    retry++;
    Serial.print("Try: ");
    Serial.print(retry);
    Serial.print(" result: ");
    Serial.println(result);
    delay(500);
  }while(result <= 0 && retry < RETRY_COUNT);
  return result;
}

int check_face(){
  return try_process_image(false);
}

int enroll_face(){
  return try_process_image(true);
}
