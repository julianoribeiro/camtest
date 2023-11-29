#pragma once

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "sdcard.h"
#include <Arduino.h>
#include <SD.h>
#include <SD_MMC.h>

#ifndef _SDCARD_H_
#define _SDCARD_H_

void writeFile(fs::FS &fs, const char * path, const char * message);
bool init_sd_card();

#endif