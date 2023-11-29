#include <Arduino.h>
#include "esp_camera.h"
#include "screen_driver.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <SPIFFS.h>
#include "ml.h"
#include <SPI.h>
#include <dirent.h>
#include <esp_timer.h>
#include <io.h>
#include "sdcard.h"
#include "sd_defines.h"

#define CAM_MODULE_NAME "ESP-S3-EYE"
#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET -1

#define CAM_PIN_VSYNC 6
#define CAM_PIN_HREF 7
#define CAM_PIN_PCLK 13
#define CAM_PIN_XCLK 15

#define CAM_PIN_SIOD 4
#define CAM_PIN_SIOC 5

#define CAM_PIN_D0 11
#define CAM_PIN_D1 9
#define CAM_PIN_D2 8
#define CAM_PIN_D3 10
#define CAM_PIN_D4 12
#define CAM_PIN_D5 18
#define CAM_PIN_D6 17
#define CAM_PIN_D7 16

// LCD Config
#define BOARD_LCD_MOSI 47
#define BOARD_LCD_MISO -1
#define BOARD_LCD_SCK 21
#define BOARD_LCD_CS 44
#define BOARD_LCD_DC 43
#define BOARD_LCD_RST -1
#define BOARD_LCD_BL 48
#define BOARD_LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
#define BOARD_LCD_BK_LIGHT_ON_LEVEL 0
#define BOARD_LCD_BK_LIGHT_OFF_LEVEL !BOARD_LCD_BK_LIGHT_ON_LEVEL
#define BOARD_LCD_H_RES 240
#define BOARD_LCD_V_RES 240
#define BOARD_LCD_CMD_BITS 8
#define BOARD_LCD_PARAM_BITS 8
#define LCD_HOST SPI2_HOST

#define COR_BOX 0x001F

namespace {
  const tflite::Model* model = nullptr;
  tflite::MicroMutableOpResolver<10>* resolver = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* input = nullptr;
  TfLiteTensor* output = nullptr;

  int tensor_arena_size = 256 * 1024;
  uint8_t* tensor_arena = nullptr;
}

static camera_config_t camera_config = {
    .pin_pwdn  = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 20971520,//EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB565,//YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_240X240,//QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 60, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1, //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY //CAMERA_GRAB_WHEN_EMPTY or CAMERA_GRAB_LATEST. Sets when buffers should be filled
};

scr_driver_t lcd; 

esp_err_t camera_init(){
    //power up the camera if PWDN pin is defined
    if(CAM_PIN_PWDN != -1){
        pinMode(CAM_PIN_PWDN, OUTPUT);
        digitalWrite(CAM_PIN_PWDN, LOW);
    }

    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        printf("Camera Init Failed\n");
        return err;
    }

    return ESP_OK;
}

esp_err_t lcd_init() {
    
    scr_info_t lcd_info;
    
    spi_config_t bus_conf = {
            .miso_io_num = (gpio_num_t)BOARD_LCD_MISO,
            .mosi_io_num = (gpio_num_t)BOARD_LCD_MOSI,
            .sclk_io_num = (gpio_num_t)BOARD_LCD_SCK,
            .max_transfer_sz = 2 * 240 * 240 + 10,
        };
    spi_bus_handle_t spi_bus = spi_bus_create(SPI2_HOST, &bus_conf);

    scr_interface_spi_config_t spi_lcd_cfg = {
            .spi_bus = spi_bus,
            .pin_num_cs = BOARD_LCD_CS,
            .pin_num_dc = BOARD_LCD_DC,
            .clk_freq = 40 * 1000000,
            .swap_data = 0,
    };

    scr_interface_driver_t *iface_drv;
    scr_interface_create(SCREEN_IFACE_SPI, &spi_lcd_cfg, &iface_drv);

    if (ESP_OK != scr_find_driver(SCREEN_CONTROLLER_ST7789, &lcd)) { 
        printf("Falha ao carregar o driver\n");
    };

    scr_controller_config_t lcd_cfg = {
        .interface_drv = iface_drv,
        .pin_num_rst = BOARD_LCD_RST,
        .pin_num_bckl = BOARD_LCD_BL,
        .rst_active_level = 0,
        .bckl_active_level = 0,
        .width = 240,
        .height = 240,
        .offset_hor = 0,
        .offset_ver = 0,
        .rotate = (scr_dir_t)0
    };    

    if (ESP_FAIL == lcd.init(&lcd_cfg)) {
        printf("screen initialize fail\n\n");
    }

    lcd.get_info(&lcd_info);
    printf("Screen name:%s | width:%d | height:%d\n", lcd_info.name, lcd_info.width, lcd_info.height);
    return ESP_OK;
}

void getPictureName(int64_t numero, char resultado[11]) {
    char str[21]; // Tamanho 21 para acomodar o valor máximo de um int64_t
    snprintf(str, sizeof(str), "%" PRId64, numero);

    int length = strlen(str);
    if (length >= 10) {
        strcpy(resultado, str + length - 10);
    } else {
        strcpy(resultado, str);
    }
}

esp_err_t camera_capture(){
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        printf("Camera Capture Failed\n");
        return ESP_FAIL;
    }
    printf("W: %d, H: %d, Size: %d\n", fb->width, fb->height, fb->len);

    // float* float_image_data = (float*)malloc(lenet_image_size * 4);
    // preprocessImageData((uint16_t*)fb->buf, fb->len, float_image_data);
    // prediction = predict(interpreter, float_image_data, fb->len);

    // free(float_image_data);
    delay(1000);
    int64_t start = esp_timer_get_time();

    char filename[30] = "/";
    char ultimos10[11];
    getPictureName(start, ultimos10);
    strcat(filename, ultimos10);
    strcat(filename, ".bin");
    writeFile(SD_MMC, filename, (char *)fb->buf);
    printf("Arquivo gerado: %s\n", filename);


    printf("Imprime imagem no LCD\n");
    lcd.draw_bitmap(0, 0, fb->width, fb->height, (uint16_t*)fb->buf);

    esp_camera_fb_return(fb);
    return ESP_OK;
}

void draw_box_number() {
    printf("desenho um quadro\n");

    int x_initial = 50;
    int y_initial = 50;
    int box_size = 96;

    for (int x = x_initial; x < x_initial + box_size; x++) {
        lcd.draw_pixel(x, y_initial, COR_BOX);
        lcd.draw_pixel(x, y_initial + box_size, COR_BOX);
    }

    for (int y = y_initial; y < y_initial + box_size; y++) {
        lcd.draw_pixel(x_initial, y, COR_BOX);
        lcd.draw_pixel(x_initial + box_size, y, COR_BOX);
    }
}

void updateProgressBar(int progress, int total) {
    const int barWidth = 80;
    float percentage = (float)progress / total;
    int pos = (int)(barWidth * percentage);

    printf("[");
    for (int i = 0; i < barWidth; i++) {
        if (i < pos) {
            printf("=");
        } else {
            printf(" ");
        }
    }
    printf("] %.2f%%\r", percentage * 100);
    fflush(stdout);
}

void predictData() {
  char test_dir[30] = "/spiffs/test";
  printf("Opening test dir %s\n", test_dir);

  // Open the directory
  DIR* dir = NULL;
  dir = opendir(test_dir);
  if (dir == NULL) {
    printf("Failed to open test dir\n");
    return;
  }

  // Iterate through each entry in the directory
  struct dirent* entry;
  int number_of_files = 10;
  int file_number = 0;
  char image_file[30];
  int lenet_image_size = 28 * 28 * 1;
  int squeeze_image_size = 32 * 32 * 1;
  uint8_t* image_data = (uint8_t*)malloc(lenet_image_size);
  uint8_t* image_32_data = (uint8_t*)malloc(squeeze_image_size);
  float* float_image_data = (float*)malloc(lenet_image_size * 4);
  float* float_image_32_data = (float*)malloc(squeeze_image_size * 4);
  int prediction;
  int predictions[100] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  int64_t start = esp_timer_get_time();
  while ((entry = readdir(dir)) != NULL) {
    snprintf(image_file, 30, "%s/%s", test_dir, entry->d_name);
    printf("\nFile: %s\n", image_file);
    read_file_data(image_file, lenet_image_size, image_data);

    // Squeeze needs 32 x 32 images
    // resizeImage(image_data, image_32_data);
    // preprocessImageData(image_32_data, squeeze_image_size, float_image_32_data);
    // prediction = predict(interpreter, float_image_32_data, squeeze_image_size);

    // Other models use 28 x 28 images
    preprocessImageData(image_data, lenet_image_size, float_image_data);
    prediction = predict(interpreter, float_image_data, lenet_image_size);

    printf("Prediction: %i\n", entry->d_name, prediction);
    include_prediction_in_confusion_matrix(predictions, entry->d_name, prediction);
    updateProgressBar(file_number++, number_of_files);
  }

  int64_t stop = esp_timer_get_time();
  printf("\n\nElapsed time: %lld microseconds\n", stop - start);

  print_confusion_matrix(predictions);

  print_available_memory();

  free(image_data);
  free(image_32_data);
  free(float_image_data);
  free(float_image_32_data);

  // Close the directory
//   closedir(dir);
//   printf("\nTest dir closed\n\n");

  delay(10000);
}

void setup() {
    delay(1000);
    Serial.begin(115200);
    if(!SPIFFS.begin(true)){  
        Serial.println("SPIFFS Mount Failed. Formatting...");
        SPIFFS.format();
    }   

    // Manter sequencia LCD -> Camera
    lcd_init();
    camera_init();
    
    if (!init_sd_card()) {
        printf("Falha ao carregar SDCard\n");
    }

    // char model_file[31] = "/spiffs/cnn.tflite";
    // interpreter = initializeInterpreter(model_file, model, resolver, tensor_arena_size, tensor_arena);
    print_available_memory();
}

void loop() {
    camera_capture();
    draw_box_number();
    delay(1000);
    // predictData();
    // delay(1000);
}