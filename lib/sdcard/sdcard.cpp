#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "sdcard.h"
#include <Arduino.h>
#include <SD.h>
#include <SD_MMC.h>

SPIClass mySPI(0);

int clk = 39;
int cmd = 38;
int d0  = 40;

bool init_sd_card() {
    if(!SD_MMC.setPins(clk, cmd, d0)){
        printf("Pin change failed!\n");
        return false;
    }
    if(!SD_MMC.begin("/sdcard", true)){
        printf("Card Mount Failed\n");
        return false;
    }

    uint8_t cardType = SD_MMC.cardType();

    if(cardType == CARD_NONE){
        printf("No SD_MMC card attached\n");
        return false;
    }

    Serial.print("SD_MMC Card Type: ");
    if(cardType == CARD_MMC){
        printf("MMC\n");
    } else if(cardType == CARD_SD){
        printf("SDSC\n");
    } else if(cardType == CARD_SDHC){
        printf("SDHC\n");
    } else {
        printf("UNKNOWN\n");
    }

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    printf("SD_MMC Card Size: %lluMB\n", cardSize);
    return true;
}

