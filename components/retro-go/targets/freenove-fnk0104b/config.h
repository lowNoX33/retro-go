// Freenove FNK0104B, 2.8-inch ILI9341 capacitive-touch display
#define RG_TARGET_NAME             "FREENOVE-FNK0104B"

// Storage
#define RG_STORAGE_ROOT             "/sd"
#define RG_STORAGE_SDMMC_HOST       SDMMC_HOST_SLOT_1
#define RG_STORAGE_SDMMC_SPEED      SDMMC_FREQ_DEFAULT

// Audio
#define RG_AUDIO_USE_INT_DAC        0
#define RG_AUDIO_USE_EXT_DAC        1
#define RG_AUDIO_USE_ES8311         1

// Display is landscape (320x240), matching Freenove's rotation 1 setup.
#define RG_SCREEN_DRIVER            0
#define RG_SCREEN_HOST              SPI2_HOST
#define RG_SCREEN_SPEED             SPI_MASTER_FREQ_40M
#define RG_SCREEN_BACKLIGHT         1
#define RG_SCREEN_WIDTH             320
#define RG_SCREEN_HEIGHT            240
#define RG_SCREEN_ROTATE            0
#define RG_SCREEN_VISIBLE_AREA      {0, 0, 0, 0}
#define RG_SCREEN_SAFE_AREA         {0, 0, 0, 0}
#define RG_SCREEN_INIT()                                                                                         \
    ILI9341_CMD(0xCF, 0x00, 0xc3, 0x30);                                                                         \
    ILI9341_CMD(0xED, 0x64, 0x03, 0x12, 0x81);                                                                   \
    ILI9341_CMD(0xE8, 0x85, 0x00, 0x78);                                                                         \
    ILI9341_CMD(0xCB, 0x39, 0x2c, 0x00, 0x34, 0x02);                                                             \
    ILI9341_CMD(0xF7, 0x20);                                                                                     \
    ILI9341_CMD(0xEA, 0x00, 0x00);                                                                               \
    ILI9341_CMD(0xC0, 0x1B);                                                                                     \
    ILI9341_CMD(0xC1, 0x12);                                                                                     \
    ILI9341_CMD(0xC5, 0x32, 0x3C);                                                                               \
    ILI9341_CMD(0xC7, 0x91);                                                                                     \
    ILI9341_CMD(0x36, 0xA8); /* MY|MV|BGR */                                                                     \
    ILI9341_CMD(0xB1, 0x00, 0x10);                                                                               \
    ILI9341_CMD(0xB6, 0x0A, 0xA2);                                                                               \
    ILI9341_CMD(0xF6, 0x01, 0x30);                                                                               \
    ILI9341_CMD(0xF2, 0x00);                                                                                     \
    ILI9341_CMD(0x26, 0x01);                                                                                     \
    ILI9341_CMD(0xE0, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00); \
    ILI9341_CMD(0xE1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F); \
    ILI9341_CMD(0x21)

// The board's BOOT button is active-low on GPIO0. External controls use a PCF8575.
#define RG_GAMEPAD_GPIO_MAP {\
    {RG_KEY_MENU,  .num = GPIO_NUM_0, .pullup = 1, .level = 0},\
}
// PCF8575 pin numbers: P00-P07 = 0-7, P10-P17 = 8-15.
// Buttons are wired between the pin and GND (pressed = 0). The PCF8575 has
// built-in weak pull-ups, so unconnected pins read as "released".
#define RG_GAMEPAD_I2C_MAP {\
    {RG_KEY_UP,     .num = 0,  .level = 0},  /* P00 */\
    {RG_KEY_DOWN,   .num = 1,  .level = 0},  /* P01 */\
    {RG_KEY_LEFT,   .num = 2,  .level = 0},  /* P02 */\
    {RG_KEY_RIGHT,  .num = 3,  .level = 0},  /* P03 */\
    {RG_KEY_START,  .num = 4,  .level = 0},  /* P04 */\
    {RG_KEY_SELECT, .num = 5,  .level = 0},  /* P05 */\
    {RG_KEY_A,      .num = 6,  .level = 0},  /* P06 */\
    {RG_KEY_B,      .num = 7,  .level = 0},  /* P07 */\
    {RG_KEY_OPTION, .num = 8,  .level = 0},  /* P10 (optional extra button) */\
}
// The Option button opens the options menu, which is where "Wi-Fi options" lives.
// If no button is wired to P10, press SELECT + START together instead.
#define RG_GAMEPAD_VIRT_MAP {\
    {RG_KEY_OPTION, RG_KEY_SELECT | RG_KEY_START},\
}
#define RG_RECOVERY_BTN             RG_KEY_MENU

// PCF8575 button expander shares the touchscreen/ES8311 I2C bus.
#define RG_I2C_GPIO_DRIVER          4   // 4 = PCF8575
#define RG_I2C_GPIO_ADDR            0x20 // A0/A1/A2 tied to GND
#define RG_I2C_SPEED                100000 // 100 kHz is more tolerant of jumper wires than 400 kHz
#define RG_GPIO_I2C_SDA             GPIO_NUM_16
#define RG_GPIO_I2C_SCL             GPIO_NUM_15

#define RG_BATTERY_DRIVER           1
#define RG_BATTERY_ADC_UNIT         ADC_UNIT_1
#define RG_BATTERY_ADC_CHANNEL      ADC_CHANNEL_8
#define RG_BATTERY_CALC_PERCENT(raw) (((raw) * 2.f - 3500.f) / (4200.f - 3500.f) * 100.f)
#define RG_BATTERY_CALC_VOLTAGE(raw) ((raw) * 2.f * 0.001f)

// Built-in battery monitor is connected to GPIO9.
// This board has no single-color GPIO LED.

// ILI9341 display pins from Freenove's FNK0104AB setup.
#define RG_GPIO_LCD_MISO            GPIO_NUM_13
#define RG_GPIO_LCD_MOSI            GPIO_NUM_11
#define RG_GPIO_LCD_CLK             GPIO_NUM_12
#define RG_GPIO_LCD_CS              GPIO_NUM_10
#define RG_GPIO_LCD_DC              GPIO_NUM_46
#define RG_GPIO_LCD_BCKL            GPIO_NUM_45

// On-board SD card uses the 1-bit SDMMC pins from Freenove's example.
#define RG_GPIO_SDSPI_CLK           GPIO_NUM_38
#define RG_GPIO_SDSPI_CMD           GPIO_NUM_40
#define RG_GPIO_SDSPI_D0            GPIO_NUM_39

// On-board ES8311 audio codec and speaker amplifier.
#define RG_GPIO_SND_I2S_MCK         GPIO_NUM_4
#define RG_GPIO_SND_I2S_BCK         GPIO_NUM_5
#define RG_GPIO_SND_I2S_DATA        GPIO_NUM_8
#define RG_GPIO_SND_I2S_WS          GPIO_NUM_7
#define RG_GPIO_SND_AMP_ENABLE      GPIO_NUM_1
#define RG_GPIO_SND_AMP_ENABLE_INVERT // Freenove's speaker amplifier enable is active-low.
