#include "FS.h"
#include "SD_MMC.h"

#define SD_MMC_CMD 38
#define SD_MMC_CLK 39
#define SD_MMC_D0  40

void init_sd(){
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  if(!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)){
    Serial.println("Card Mount Failed");
    return;
  }
}
