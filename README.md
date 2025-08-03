# CH32V003 SSD1306 OLED Display Example
Simple SSD1306 OLED Display Example using CH32V003F4P6 and CH32V003_lib_i2c.


## Background
The SSD1306 example and library that comes with the default ch32fun installation does not support I2C remapping to other pins and simultaneous use with other I2C devices.
Therefore, this repository replaces the I2C communication part with a better library by ADBeta.

##  Hardware Requirements

- **Microcontroller**: CH32V003F4P6 (or compatible CH32V003 variant)
- **Display**: SSD1306-based OLED display (128x64 or 128x32)
- **Power**: 3.3V power rail for OLED 

##  Dependencies

- **CH32V003 Fun Framework**: Base framework for CH32V003 development
- **CH32V003_lib_i2c**: I2C communication library (v5.4+)
- **PlatformIO**: Build system and development environment

##  Configuration

### Power Control Pins
⚠️ **IMPORTANT: These pins are board-specific and MUST be changed according to your hardware design!**

- **P6V0_PIN**: PA1 (6V power rail control) - **BOARD DEPENDENT - CHANGE AS NEEDED**
- **LCD_P3V3_PIN**: PD3 (3.3V LCD power control) - **BOARD DEPENDENT - CHANGE AS NEEDED**

> **Note**: If your MCU and LCD share the same 3.3V power rail, the power control functions may not be needed at all. Modify the `init_lcd_power()` function in `src/lcd_example.c` according to your specific hardware design.

### I2C Configuration

The SSD1306 device is configured in `src/lcd_example.c`:

```c
static i2c_device_t ssd1306_dev = {
    .clkr = I2C_CLK_400KHZ,     // 400kHz I2C clock
    .type = I2C_ADDR_7BIT,      // 7-bit addressing
    .addr = 0x3C,               // Standard SSD1306 I2C address
    .regb = 1,                  // 1-byte register addressing
    .tout = 2000,               // 2000ms timeout
};
```

### Display Configuration

Display resolution should be customized in `include/myssd1306.h`:
(but only tested with SSD1306_128_32 (128x32 pixels))

### Pinout Selection

Choose your I2C pinout by defining one of these in `funconfig.h`:

```c
#define I2C_PINOUT_DEFAULT    // PC2/PC1
#define I2C_PINOUT_ALT_1      // PD1/PD0
#define I2C_PINOUT_ALT_2      // PC5/PC6
```

##  Project Structure

```
CH32V003_SSD1306_Example/
├── platformio.ini              # PlatformIO configuration
├── src/
│   ├── lcd_example.c           # Main application code
│   └── myssd1306.c             # SSD1306 display driver implementation
├── include/
│   ├── myssd1306.h             # SSD1306 display driver header
│   └── font_8x8.h              # 8x8 bitmap font definitions
├── lib/
│   ├── CH32V003_lib_i2c/       # I2C library (git submodule)
│   │   ├── lib_i2c.h           # I2C library header
│   │   ├── lib_i2c.c           # I2C library implementation
│   │   └── README.md           # I2C library documentation
│   └── README                  # Library readme
├── test/                       # Test files directory
└── README.md                   # This file
```



##  License

This project is released under the MIT License. See individual library files for their respective licenses.

##  Acknowledgments

- **ADBeta**: CH32V003_lib_i2c library
- **CNLohr**: ch32fun framework
- **E. Brombaugh**: Original SSD1306 driver foundation
- **DoubleWaffleCakes**: Bomb illustration bitmap graphics

