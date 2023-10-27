#include <Arduino.h>
#include "esp_camera.h"
#include "screen_driver.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "logo_en_240x240_lcd.h"

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


#define BOARD_LCD_MOSI 47
#define BOARD_LCD_MISO -1
#define BOARD_LCD_SCK 21
#define BOARD_LCD_CS 44
#define BOARD_LCD_DC 43
#define BOARD_LCD_RST -1
#define BOARD_LCD_BL 48
#define BOARD_LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define BOARD_LCD_BK_LIGHT_ON_LEVEL 0
#define BOARD_LCD_BK_LIGHT_OFF_LEVEL !BOARD_LCD_BK_LIGHT_ON_LEVEL
#define BOARD_LCD_H_RES 240
#define BOARD_LCD_V_RES 240
#define BOARD_LCD_CMD_BITS 8
#define BOARD_LCD_PARAM_BITS 8
#define LCD_HOST SPI2_HOST

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

    .xclk_freq_hz = 20000000,//EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,//YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_SVGA,//QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 30, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 2, //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_LATEST //CAMERA_GRAB_WHEN_EMPTY or CAMERA_GRAB_LATEST. Sets when buffers should be filled
};

scr_driver_t lcd;

esp_err_t camera_init(){
    //power up the camera if PWDN pin is defined
    if(CAM_PIN_PWDN != -1){
        pinMode(CAM_PIN_PWDN, OUTPUT);
        digitalWrite(CAM_PIN_PWDN, LOW);
    }
    if (psramFound()){ 
        printf("Com PSRAM\n");
    } else {
        printf("Sem PSRAM\n");
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
    
    scr_driver_t lcd;
    scr_info_t lcd_info;
    
    printf("Criando barramento\n");
    spi_config_t bus_conf = {
            .miso_io_num = (gpio_num_t)BOARD_LCD_MISO,
            .mosi_io_num = (gpio_num_t)BOARD_LCD_MOSI,
            .sclk_io_num = (gpio_num_t)BOARD_LCD_SCK,
            .max_transfer_sz = 2 * 240 * 240 + 10,
        };
    spi_bus_handle_t spi_bus = spi_bus_create(SPI2_HOST, &bus_conf);
    printf("Barramento Criado\n");

    scr_interface_spi_config_t iface_cfg = {
        .spi_bus = spi_bus,
        .pin_num_cs = BOARD_LCD_CS,
        .pin_num_dc = BOARD_LCD_DC,
        .clk_freq = 40 * 1000000,
        .swap_data = 0,
    };

    printf("Interface inicio\n");
    scr_interface_driver_t *iface_drv;
    scr_interface_create(SCREEN_IFACE_SPI, &iface_cfg, &iface_drv);
    printf("Interface fim\n");

    printf("Driver inicio\n");
    if (ESP_OK != scr_find_driver(SCREEN_CONTROLLER_ST7789, &lcd)) { 
        printf("Falha ao carregar o driver\n");
    };
    printf("Driver Fim\n");

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
        .rotate = (scr_dir_t)0,
    };    
    lcd.init(&lcd_cfg);

    return ESP_OK;
    // TEST_ASSERT(ESP_OK == lcd.get_info(&lcd_info));
}

esp_err_t camera_capture(){
    //acquire a frame
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        printf("Camera Capture Failed\n");
        return ESP_FAIL;
    }
    //replace this with your own function
    printf("W: %d, H: %d, F: %d\n", fb->width, fb->height, fb->format);

    uint16_t *pixels = (uint16_t *)heap_caps_malloc((logo_en_240x240_lcd_width * logo_en_240x240_lcd_height) * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    memcpy(pixels, logo_en_240x240_lcd, (logo_en_240x240_lcd_width * logo_en_240x240_lcd_height) * sizeof(uint16_t));
    lcd.draw_bitmap(0, 0, logo_en_240x240_lcd_width, logo_en_240x240_lcd_height, (uint16_t *)pixels);
    heap_caps_free(pixels);
    
    //return the frame buffer back to the driver for reuse
    esp_camera_fb_return(fb);
    return ESP_OK;
}

void setup() {
  delay(3000);
  Serial.begin(115200);
  printf("Init LCD\n");
  lcd_init();
  printf("Camera Init\n");
  camera_init();
  delay(1000);
  printf("Camera Capture\n");
  camera_capture();
  delay(30000);
  printf("End\n");
}

void loop() {
}