/* ============================================================================
 * LCD Example and Test Functions
 * ChocoPI OLED Display Demonstration
 * 
 * This file contains comprehensive test and demonstration functions for the
 * SSD1306 OLED display, showcasing various graphics capabilities including
 * pixel manipulation, drawing primitives, text rendering, and image display.
 * ============================================================================ */

#include <stdio.h>
#include "ch32fun.h"
#include "myssd1306.h"
#include "lib_i2c.h"

/* ============================================================================
 * DEVICE CONFIGURATION
 * ============================================================================ */

// I2C device configuration for SSD1306 OLED display
static i2c_device_t ssd1306_dev = {
	.clkr = I2C_CLK_400KHZ,     // 400kHz I2C clock
	.type = I2C_ADDR_7BIT,      // 7-bit addressing
	.addr = 0x3C,               // Standard SSD1306 I2C address
	.regb = 1,                  // 1-byte register addressing
	.tout = 2000,               // 2000ms timeout
};

/* ============================================================================
 * GRAPHICS ASSETS
 * ============================================================================ */

/*
 * Bomb illustration bitmap (32x32 pixels)
 * Graphics provided by DoubleWaffleCakes (Reddit user)
 * Used for demonstrating bitmap rendering capabilities
 */
const unsigned char bomb_i_stripped[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00,
  0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x48, 0x00,
  0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x03, 0xfe, 0x00,
  0x00, 0x0f, 0xff, 0x80, 0x00, 0x1f, 0xff, 0xc0, 0x00, 0x1f, 0xff, 0xc0,
  0x00, 0x3f, 0xff, 0xe0, 0x00, 0x3f, 0xff, 0xe0, 0x00, 0x73, 0x9f, 0xf0,
  0x00, 0x73, 0x9f, 0xf0, 0x00, 0x73, 0x9f, 0xf0, 0x00, 0x73, 0x9f, 0xf0,
  0x00, 0x73, 0x9f, 0xf0, 0x00, 0x3f, 0xff, 0xe0, 0x00, 0x3f, 0xff, 0xe0,
  0x00, 0x1f, 0xff, 0xc0, 0x00, 0x1f, 0xff, 0xc0, 0x00, 0x0f, 0xff, 0x80,
  0x00, 0x13, 0xe0, 0x40, 0x00, 0x20, 0xc0, 0x20, 0x00, 0x7f, 0xff, 0xe0,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
const unsigned int bomb_i_stripped_len = 128;

/* ============================================================================
 * POWER MANAGEMENT FUNCTIONS
 * ============================================================================ */

/**
 * @brief Initialize power rails for LCD operation
 *
 * This function is totally dependent on the hardware design.
 * For example, if MCU and LCD are applied to the same 3V3 power rail, this function could be empty.
 * 
 */
#define LCD_P3V3_PIN       PD3  // 3.3V LCD power control (enable after 6V)
#define P6V0_PIN           PA1  // 6V power rail control (enable first)
static void init_lcd_power(void)
{
	// Configure power control pins
	funPinMode(P6V0_PIN, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
	funPinMode(LCD_P3V3_PIN, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);

	// Enable power rails in proper sequence
	funDigitalWrite(P6V0_PIN, FUN_HIGH);       // 6V rail first
	// Wait for pin configuration to settle
	Delay_Ms(100);
	funDigitalWrite(LCD_P3V3_PIN, FUN_HIGH);   // 3.3V rail second
	// Wait for pin configuration to settle
	Delay_Ms(100);
}

/* ============================================================================
 * TEST MODE FUNCTIONS
 * ============================================================================ */

/**
 * @brief Test mode 0: Binary buffer fill pattern
 */
static void test_binary_pattern(void)
{
	printf("buffer fill with binary\n\r");
	for (int i = 0; i < sizeof(ssd1306_buffer); i++) {
		ssd1306_buffer[i] = i;
	}
}

/**
 * @brief Test mode 1: Pixel plotting demonstration
 */
static void test_pixel_plots(void)
{
	printf("pixel plots\n\r");
	for (int i = 0; i < SSD1306_W; i++) {
		ssd1306_drawPixel(i, i / (SSD1306_W / SSD1306_H), 1);
		ssd1306_drawPixel(i, SSD1306_H - 1 - (i / (SSD1306_W / SSD1306_H)), 1);
	}
}

/**
 * @brief Test mode 2: Line drawing demonstration
 */
static void test_line_plots(void)
{
	printf("Line plots\n\r");
	uint8_t y = 0;
	for (uint8_t x = 0; x < SSD1306_W; x += 16) {
		ssd1306_drawLine(x, 0, SSD1306_W, y, 1);
		ssd1306_drawLine(SSD1306_W - x, SSD1306_H, 0, SSD1306_H - y, 1);
		y += SSD1306_H / 8;
	}
}

/**
 * @brief Test mode 3: Circle drawing (outline and filled)
 */
static void test_circles(void)
{
	printf("Circles empty and filled\n\r");
	for (uint8_t x = 0; x < SSD1306_W; x += 16) {
		if (x < 64) {
			ssd1306_drawCircle(x, SSD1306_H / 2, 15, 1);
		} else {
			ssd1306_fillCircle(x, SSD1306_H / 2, 15, 1);
		}
	}
}

/**
 * @brief Test mode 4: Bitmap image rendering
 */
static void test_image_display(void)
{
	printf("Image\n\r");
	ssd1306_drawImage(0, 0, bomb_i_stripped, 32, 32, 0);
}

/**
 * @brief Test mode 5: Standard text rendering with XOR highlight
 */
static void test_unscaled_text(void)
{
	printf("Unscaled Text\n\r");
	ssd1306_drawstr(0, 0, "This is a test", 1);
	ssd1306_drawstr(0, 8, "of the emergency", 1);
	ssd1306_drawstr(0, 16, "broadcasting", 1);
	ssd1306_drawstr(0, 24, "system.", 1);
	
	// Additional text for taller displays
	if (SSD1306_H > 32) {
		ssd1306_drawstr(0, 32, "Lorem ipsum", 1);
		ssd1306_drawstr(0, 40, "dolor sit amet,", 1);
		ssd1306_drawstr(0, 48, "consectetur", 1);
		ssd1306_drawstr(0, 56, "adipiscing", 1);
	}
	
	// Demonstrate XOR highlighting
	ssd1306_xorrect(SSD1306_W / 2, 0, SSD1306_W / 2, SSD1306_W);
}

/**
 * @brief Test mode 6: Scaled text (8x8 and 16x16)
 */
static void test_scaled_text_small(void)
{
	printf("Scaled Text 1, 2\n\r");
	ssd1306_drawstr_sz(0, 0, "sz 8x8", 1, fontsize_8x8);
	ssd1306_drawstr_sz(0, 16, "16x16", 1, fontsize_16x16);
}

/**
 * @brief Test mode 7: Large scaled text (32x32)
 */
static void test_scaled_text_medium(void)
{
	printf("Scaled Text 4\n\r");
	ssd1306_drawstr_sz(0, 0, "32x32", 1, fontsize_32x32);
}

/**
 * @brief Test mode 8: Extra large scaled text (64x64)
 */
static void test_scaled_text_large(void)
{
	printf("Scaled Text 8\n\r");
	ssd1306_drawstr_sz(0, 0, "64", 1, fontsize_64x64);
}

/* ============================================================================
 * MAIN EXAMPLE FUNCTION
 * ============================================================================ */

int main(void)
{
    // Initialize system
    SystemInit();
	funGpioInitAll();
	
	// Initialize LCD power supply
	init_lcd_power();

	// Initialize I2C and display
	// Wait for I2C device to settle after power-up
	Delay_Ms(100);
	
	i2c_init(&ssd1306_dev);
	ssd1306_init(&ssd1306_dev);

	// Main demonstration loop
	uint8_t mode = 0;
	const uint8_t max_modes = (SSD1306_H > 32) ? 9 : 8;
	
	while(1) {
		// Clear display buffer for next demonstration
		ssd1306_setbuf(0);

		// Execute current test mode
		switch (mode) {
			case 0: test_binary_pattern();     break;
			case 1: test_pixel_plots();        break;
			case 2: test_line_plots();         break;
			case 3: test_circles();            break;
			case 4: test_image_display();      break;
			case 5: test_unscaled_text();      break;
			case 6: test_scaled_text_small();  break;
			case 7: test_scaled_text_medium(); break;
			case 8: test_scaled_text_large();  break;
			default: break;
		}
		
		// Update display and advance to next mode
		ssd1306_refresh(&ssd1306_dev);
		mode = (mode + 1) % max_modes;

		// Wait before next demonstration
		Delay_Ms(1000);
	}
	
    return 0;
}