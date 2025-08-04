/* ============================================================================
 * SSD1306 OLED Display Driver Header
 * Single-File-Header for I2C OLED Display Control
 * 
 * Originally by E. Brombaugh (05-05-2023), enhanced for ChocoPI
 * 
 * This header provides a complete interface for controlling SSD1306-based
 * OLED displays via I2C, including graphics primitives, text rendering,
 * and buffer management.
 * ============================================================================ */

#ifndef _SSD1306_H
#define _SSD1306_H

#include <stdint.h>
#include <string.h>
#include "font_8x8.h"
#include "lib_i2c.h"

/* ============================================================================
 * DISPLAY CONFIGURATION
 * ============================================================================ */

// Optimal packet size for I2C communication with this OLED
#define SSD1306_PSZ 32

#if defined (SSD1306_CUSTOM)
// Let the caller configure the OLED dimensions
#else
// Standard display configurations - one must be defined
#if !defined (SSD1306_64X32) && !defined (SSD1306_72X40) && !defined (SSD1306_128X32) && !defined (SSD1306_128X64) && !defined (SH1107_128x128) && !(defined(SSD1306_W) && defined(SSD1306_H) && defined(SSD1306_OFFSET) )
	#error "Please define the SSD1306_WXH resolution used in your application"
#endif

#ifdef SSD1306_64X32
#define SSD1306_W 64
#define SSD1306_H 32
#define SSD1306_OFFSET 32
#endif

#ifdef SSD1306_72X40
#define SSD1306_W 72
#define SSD1306_H 40
#define SSD1306_OFFSET 28
#endif

#ifdef SSD1306_128X32
#define SSD1306_W 128
#define SSD1306_H 32
#define SSD1306_OFFSET 0
#endif

#ifdef SSD1306_128X64
#define SSD1306_W 128
#define SSD1306_H 64
#define SSD1306_OFFSET 0
#endif

#ifdef SH1107_128x128
#define SH1107
#define SSD1306_W 128
#define SSD1306_H 128
#define SSD1306_OFFSET 0
#endif

#endif

/* ============================================================================
 * SSD1306 COMMAND DEFINITIONS
 * ============================================================================ */

// Display control commands
#define SSD1306_DISPLAYOFF          0xAE  // Turn display off
#define SSD1306_DISPLAYON           0xAF  // Turn display on
#define SSD1306_NORMALDISPLAY       0xA6  // Normal display mode
#define SSD1306_INVERTDISPLAY       0xA7  // Inverted display mode
#define SSD1306_DISPLAYALLON_RESUME 0xA4  // Resume display from RAM
#define SSD1306_DISPLAYALLON        0xA5  // Entire display on

// Contrast and brightness
#define SSD1306_SETCONTRAST         0x81  // Set contrast control

// Memory addressing
#define SSD1306_MEMORYMODE          0x20  // Set memory addressing mode
#define SSD1306_COLUMNADDR          0x21  // Set column address range
#define SSD1306_PAGEADDR            0x22  // Set page address range
#define SSD1306_SETLOWCOLUMN        0x00  // Set lower column start address
#define SSD1306_SETHIGHCOLUMN       0x10  // Set higher column start address
#define SSD1306_SETSTARTLINE        0x40  // Set start line address

// Hardware configuration
#define SSD1306_SEGREMAP            0xA0  // Set segment re-map
#define SSD1306_SETMULTIPLEX        0xA8  // Set multiplex ratio
#define SSD1306_SETDISPLAYOFFSET    0xD3  // Set display offset
#define SSD1306_SETCOMPINS          0xDA  // Set COM pins hardware configuration
#define SSD1306_COMSCANINC          0xC0  // Set COM output scan direction (normal)
#define SSD1306_COMSCANDEC          0xC8  // Set COM output scan direction (remapped)

// Timing and driving
#define SSD1306_SETDISPLAYCLOCKDIV  0xD5  // Set display clock divide ratio
#define SSD1306_SETPRECHARGE        0xD9  // Set pre-charge period
#define SSD1306_SETVCOMDETECT       0xDB  // Set VCOMH deselect level
#define SSD1306_CHARGEPUMP          0x8D  // Charge pump setting

// Power supply configuration
#define SSD1306_EXTERNALVCC         0x1   // External VCC
#define SSD1306_SWITCHCAPVCC        0x2   // Switched capacitor VCC

// Command terminator
#define SSD1306_TERMINATE_CMDS      0xFF  // Command sequence terminator

// VCC mode selection
#define vccstate SSD1306_SWITCHCAPVCC

/* ============================================================================
 * FONT SIZE ENUMERATION
 * ============================================================================ */

typedef enum {
    fontsize_8x8   = 1,  // 8x8 pixel characters (1x scale)
    fontsize_16x16 = 2,  // 16x16 pixel characters (2x scale)
    fontsize_32x32 = 4,  // 32x32 pixel characters (4x scale)
    fontsize_64x64 = 8,  // 64x64 pixel characters (8x scale)
} font_size_t;

/* ============================================================================
 * EXTERNAL VARIABLES
 * ============================================================================ */

extern uint8_t ssd1306_buffer[SSD1306_W * SSD1306_H / 8];  // Display buffer

/* ============================================================================
 * INITIALIZATION AND CONTROL FUNCTIONS
 * ============================================================================ */

/**
 * @brief Initialize SSD1306 display
 * @param dev I2C device structure pointer
 * @return 0 on success, non-zero on error
 */
uint8_t ssd1306_init(i2c_device_t *dev);

/**
 * @brief Reset display (if reset pin available)
 */
void ssd1306_rst(void);

/**
 * @brief Send command to display
 * @param dev I2C device structure pointer
 * @param cmd Command byte to send
 * @return 0 on success, non-zero on error
 */
uint8_t ssd1306_cmd(i2c_device_t *dev, uint8_t cmd);

/**
 * @brief Send data to display
 * @param dev I2C device structure pointer
 * @param data Data buffer to send
 * @param sz Size of data buffer
 * @return 0 on success, non-zero on error
 */
uint8_t ssd1306_data(i2c_device_t *dev, uint8_t *data, int sz);

/* ============================================================================
 * BUFFER MANAGEMENT FUNCTIONS
 * ============================================================================ */

/**
 * @brief Clear or fill entire display buffer
 * @param color 0 for black (clear), 1 for white (fill)
 */
void ssd1306_setbuf(uint8_t color);

/**
 * @brief Refresh display from buffer
 * @param dev I2C device structure pointer
 */
void ssd1306_refresh(i2c_device_t *dev);

/* ============================================================================
 * PIXEL MANIPULATION FUNCTIONS
 * ============================================================================ */

/**
 * @brief Set pixel in buffer
 * @param x X coordinate (0 to SSD1306_W-1)
 * @param y Y coordinate (0 to SSD1306_H-1)
 * @param color 0 for black, 1 for white
 */
void ssd1306_drawPixel(uint32_t x, uint32_t y, int color);

/**
 * @brief XOR pixel in buffer (toggle pixel state)
 * @param x X coordinate (0 to SSD1306_W-1)
 * @param y Y coordinate (0 to SSD1306_H-1)
 */
void ssd1306_xorPixel(uint32_t x, uint32_t y);

/* ============================================================================
 * DRAWING PRIMITIVE FUNCTIONS
 * ============================================================================ */

/**
 * @brief Draw vertical line
 * @param x X coordinate
 * @param y Starting Y coordinate
 * @param h Height in pixels
 * @param color Line color (0 or 1)
 */
void ssd1306_drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color);

/**
 * @brief Draw horizontal line
 * @param x Starting X coordinate
 * @param y Y coordinate
 * @param w Width in pixels
 * @param color Line color (0 or 1)
 */
void ssd1306_drawFastHLine(uint32_t x, uint32_t y, uint32_t w, uint32_t color);

/**
 * @brief Draw line between two points
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 * @param color Line color (0 or 1)
 */
void ssd1306_drawLine(int x0, int y0, int x1, int y1, uint32_t color);

/**
 * @brief Draw circle outline
 * @param x Center X coordinate
 * @param y Center Y coordinate
 * @param radius Circle radius
 * @param color Line color (0 or 1)
 */
void ssd1306_drawCircle(int x, int y, int radius, int color);

/**
 * @brief Draw filled circle
 * @param x Center X coordinate
 * @param y Center Y coordinate
 * @param radius Circle radius
 * @param color Fill color (0 or 1)
 */
void ssd1306_fillCircle(int x, int y, int radius, int color);

/**
 * @brief Draw rectangle outline
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param w Width in pixels
 * @param h Height in pixels
 * @param color Line color (0 or 1)
 */
void ssd1306_drawRect(int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color);

/**
 * @brief Draw filled rectangle
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param w Width in pixels
 * @param h Height in pixels
 * @param color Fill color (0 or 1)
 */
void ssd1306_fillRect(uint32_t x, uint32_t y, uint8_t w, uint32_t h, uint32_t color);

/**
 * @brief XOR rectangle (invert rectangular area)
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param w Width in pixels
 * @param h Height in pixels
 */
void ssd1306_xorrect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/* ============================================================================
 * IMAGE RENDERING FUNCTIONS
 * ============================================================================ */

/**
 * @brief Draw bitmap image
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param input Bitmap data pointer
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @param color_mode Color mode (0 or 1)
 */
void ssd1306_drawImage(uint32_t x, uint32_t y, const unsigned char* input, uint32_t width, uint32_t height, uint32_t color_mode);

/* ============================================================================
 * TEXT RENDERING FUNCTIONS
 * ============================================================================ */

/**
 * @brief Draw single character (8x8 pixels)
 * @param x X coordinate
 * @param y Y coordinate
 * @param chr Character to draw
 * @param color Text color (0 or 1)
 */
void ssd1306_drawchar(uint8_t x, uint8_t y, uint8_t chr, uint8_t color);

/**
 * @brief Draw string (8x8 pixel characters)
 * @param x Starting X coordinate
 * @param y Y coordinate
 * @param str String to draw
 * @param color Text color (0 or 1)
 */
void ssd1306_drawstr(uint8_t x, uint8_t y, char *str, uint8_t color);

/**
 * @brief Draw single character with scaling
 * @param x X coordinate
 * @param y Y coordinate
 * @param chr Character to draw
 * @param color Text color (0 or 1)
 * @param font_size Font scaling factor
 */
void ssd1306_drawchar_sz(uint8_t x, uint8_t y, uint8_t chr, uint8_t color, font_size_t font_size);

/**
 * @brief Draw string with scaling
 * @param x Starting X coordinate
 * @param y Y coordinate
 * @param str String to draw
 * @param color Text color (0 or 1)
 * @param font_size Font scaling factor
 */
void ssd1306_drawstr_sz(uint8_t x, uint8_t y, char *str, uint8_t color, font_size_t font_size);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * @brief Calculate absolute value
 * @param x Input value
 * @return Absolute value of x
 */
int gfx_abs(int x);

/**
 * @brief Swap two integer values
 * @param z0 Pointer to first value
 * @param z1 Pointer to second value
 */
void gfx_swap(int *z0, int *z1);

#endif // _SSD1306_H
