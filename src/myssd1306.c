/* ============================================================================
 * SSD1306 OLED Display Driver Implementation
 * Complete Graphics Library for I2C OLED Displays
 * 
 * Originally by E. Brombaugh (05-05-2023), enhanced for ChocoPI
 * 
 * This implementation provides a complete graphics library for SSD1306-based
 * OLED displays, including initialization, buffer management, drawing
 * primitives, text rendering, and display update functions.
 * 
 * Features:
 * - Multiple display size support (64x32, 72x40, 128x32, 128x64, SH1107)
 * - Efficient buffer-based rendering
 * - Complete set of drawing primitives (pixels, lines, circles, rectangles)
 * - Text rendering with multiple font sizes
 * - Bitmap image display
 * - XOR operations for highlighting and animation
 * ============================================================================ */

#include "myssd1306.h"

/* ============================================================================
 * DISPLAY INITIALIZATION ARRAYS
 * ============================================================================ */



#if !defined(SSD1306_CUSTOM_INIT_ARRAY) || !SSD1306_CUSTOM_INIT_ARRAY
// OLED initialization commands for 128x32
const uint8_t ssd1306_init_array[] =
{
#ifdef SH1107
	SSD1306_DISPLAYOFF,               // Turn OLED off
	0x00,                             // Low column
	0x10,                             // High column
	0xb0,                             // Page address
	0xdc, 0x00,                       // Set Display Start Line  (Where in memory it reads from)
	SSD1306_SETCONTRAST, 0x6f,        // Set constrast
	SSD1306_COLUMNADDR,               // Set memory addressing mode
	SSD1306_DISPLAYALLON_RESUME,      // normal (as opposed to invert colors, always on or off.)
	SSD1306_SETMULTIPLEX, (SSD1306_H-1), // Iterate over all 128 rows (Multiplex Ratio)
	SSD1306_SETDISPLAYOFFSET, 0x00,   // Set display offset // Where this appears on-screen  (Some displays will be different)
	SSD1306_SETDISPLAYCLOCKDIV, 0xf0, // Set precharge properties.  THIS IS A LIE  This has todo with timing.  <<< This makes it go brrrrrrrrr
	SSD1306_SETPRECHARGE, 0x1d,       // Set pre-charge period  (This controls brightness)
	SSD1306_SETVCOMDETECT, 0x35,      // Set vcomh
	SSD1306_SETSTARTLINE | 0x0,            // 0x40 | line
	0xad, 0x80,                       // Set Charge pump
	SSD1306_SEGREMAP, 0x01,           // Default mapping
	SSD1306_SETPRECHARGE, 0x06,       // ???? No idea what this does, but this looks best.
	SSD1306_SETCONTRAST, 0xfe,        // Set constrast
	SSD1306_SETVCOMDETECT, 0xfe,      // Set vcomh
	SSD1306_SETMULTIPLEX, (SSD1306_H-1), // 128-wide.
	SSD1306_DISPLAYON, // Display on.
#else
    SSD1306_DISPLAYOFF,                    // 0xAE
    SSD1306_SETDISPLAYCLOCKDIV,            // 0xD5
    0x80,                                  // the suggested ratio 0x80
    SSD1306_SETMULTIPLEX,                  // 0xA8
#if defined(SSD1306_64X32)
	0x1F,                                  // for 64-wide displays
#elif defined(SSD1306_72X40)
	0x27,
#else
	0x3F,                                  // for 128-wide displays
#endif
    SSD1306_SETDISPLAYOFFSET,              // 0xD3
    0x00,                                  // no offset
	SSD1306_SETSTARTLINE | 0x0,            // 0x40 | line
    SSD1306_CHARGEPUMP,                    // 0x8D
	0x14,                                  // enable?
    SSD1306_MEMORYMODE,                    // 0x20
    0x00,                                  // 0x0 act like ks0108
    SSD1306_SEGREMAP | 0x1,                // 0xA0 | bit
    SSD1306_COMSCANDEC,
    SSD1306_SETCOMPINS,                    // 0xDA
#ifdef SSD1306_128X32
	0x22,
#else
	0x12,                                  //
#endif

    SSD1306_SETCONTRAST,                   // 0x81
#ifdef SSD1306_72X40
	0xAF,
#else
	0x8F,
#endif
    SSD1306_SETPRECHARGE,                  // 0xd9
	0xF1,
    SSD1306_SETVCOMDETECT,                 // 0xDB
    0x40,
    SSD1306_DISPLAYALLON_RESUME,           // 0xA4
#ifndef SSD1327
    SSD1306_NORMALDISPLAY,                 // 0xA6
#endif
	SSD1306_DISPLAYON,	                   // 0xAF --turn on oled panel
#endif
	SSD1306_TERMINATE_CMDS                 // 0xFF --fake command to mark end
};
#endif

static const uint8_t cmd_array[] = {
			//0x3C << 1, // 0x3C = I2C address
			0x80, // 0 = next byte is command
			SSD1306_COLUMNADDR,
			0x80, // 0 = next byte is command
			SSD1306_OFFSET,   // Column start address (0 = reset)
			0x80, // 0 = next byte is command
			SSD1306_OFFSET+SSD1306_W-1, // Column end address (127 = reset)
			0x80, // 0 = next byte is command
			SSD1306_PAGEADDR,
			0x80, // 0 = next byte is command
			0, // Page start address (0 = reset)
			0x80, // 0 = next byte is command
			7, // Page end address
			0x40, // Hereafter, all bytes are data
		};

// the display buffer
uint8_t ssd1306_buffer[SSD1306_W * SSD1306_H / 8];
uint8_t display_cmdbuffer[sizeof(cmd_array)+ (SSD1306_W*SSD1306_H/8)]; // + display buffer

/*
 * reset is not used for SSD1306 I2C interface
 */
void ssd1306_rst(void)
{
}

/*
 * send OLED command byte
 */
uint8_t ssd1306_cmd(i2c_device_t *dev, uint8_t cmd)
{
	return (uint8_t)i2c_write_raw(dev, (uint8_t[]){0x00, cmd}, 2);
}

/*
 * send OLED data packet (up to 32 bytes)
 */
uint8_t ssd1306_data(i2c_device_t *dev, uint8_t *data, int sz)
{
	uint8_t pkt[SSD1306_PSZ + 1]; // +1 for command byte

	/* build data packet */
	if(sz > SSD1306_PSZ) sz = SSD1306_PSZ; // limit to max packet size
	pkt[0] = 0x40; // 0 = next byte is data
	memcpy(&pkt[1], data, sz);

	return (uint8_t)i2c_write_raw(dev, pkt, sz + 1);
}

/*
 * set the buffer to a color
 */
void ssd1306_setbuf(uint8_t color)
{
	memset(ssd1306_buffer, color ? 0xFF : 0x00, 
		SSD1306_W * SSD1306_H / 8);
}

/*
 * Send the frame buffer
 */
void ssd1306_refresh(i2c_device_t *dev)
{
	uint32_t i;

	ssd1306_cmd(dev, SSD1306_COLUMNADDR);
	ssd1306_cmd(dev, SSD1306_OFFSET);   // Column start address (0 = reset)
	ssd1306_cmd(dev, SSD1306_OFFSET+SSD1306_W-1); // Column end address (127 = reset)
	
	ssd1306_cmd(dev, SSD1306_PAGEADDR);
	ssd1306_cmd(dev, 0); // Page start address (0 = reset)
	ssd1306_cmd(dev, 7); // Page end address

	/* for fully used rows just plow thru everything */
    for(i=0;i<sizeof(ssd1306_buffer);i+=SSD1306_PSZ)
	{
		/* send PSZ block of data */
		ssd1306_data(dev, &ssd1306_buffer[i], SSD1306_PSZ);
	}
}

/*
 * plot a pixel in the buffer
 */
void ssd1306_drawPixel(uint32_t x, uint32_t y, int color)
{
	uint32_t addr;
	
	/* clip */
	if(x >= SSD1306_W)
		return;
	if(y >= SSD1306_H)
		return;
	
	/* compute buffer address */
	addr = x + SSD1306_W*(y/8);
	
	/* set/clear bit in buffer */
	if(color)
		ssd1306_buffer[addr] |= (1<<(y&7));
	else
		ssd1306_buffer[addr] &= ~(1<<(y&7));
}

/*
 * plot a pixel in the buffer
 */
void ssd1306_xorPixel(uint32_t x, uint32_t y)
{
	uint32_t addr;
	
	/* clip */
	if(x >= SSD1306_W)
		return;
	if(y >= SSD1306_H)
		return;
	
	/* compute buffer address */
	addr = x + SSD1306_W*(y/8);
	
	ssd1306_buffer[addr] ^= (1<<(y&7));
}

/*
 * draw a an image from an array, directly into to the display buffer
 * the color modes allow for overwriting and even layering (sprites!)
 */
void ssd1306_drawImage(uint32_t x, uint32_t y, const unsigned char* input, uint32_t width, uint32_t height, uint32_t color_mode) {
	uint32_t x_absolute;
	uint32_t y_absolute;
	uint32_t pixel;
	uint32_t bytes_to_draw = width / 8;
	uint32_t buffer_addr;

	for (uint32_t line = 0; line < height; line++) {
		y_absolute = y + line;
		if (y_absolute >= SSD1306_H) {
			break;
		}

		// SSD1306 is in vertical mode, yet we want to draw horizontally, which necessitates assembling the output bytes from the input data
		// bitmask for current pixel in vertical (output) byte
		uint32_t v_mask = 1 << (y_absolute & 7);

		for (uint32_t byte = 0; byte < bytes_to_draw; byte++) {
			uint32_t input_byte = input[byte + line * bytes_to_draw];

			for (pixel = 0; pixel < 8; pixel++) {
				x_absolute = x + 8 * (bytes_to_draw - byte) + pixel;
				if (x_absolute >= SSD1306_W) {
					break;
				}
				// looking at the horizontal display, we're drawing bytes bottom to top, not left to right, hence y / 8
				buffer_addr = x_absolute + SSD1306_W * (y_absolute / 8);
				// state of current pixel
				uint8_t input_pixel = input_byte & (1 << pixel);

				switch (color_mode) {
					case 0:
						// write pixels as they are
						ssd1306_buffer[buffer_addr] = (ssd1306_buffer[buffer_addr] & ~v_mask) | (input_pixel ? v_mask : 0);
						break;
					case 1:
						// write pixels after inversion
						ssd1306_buffer[buffer_addr] = (ssd1306_buffer[buffer_addr] & ~v_mask) | (!input_pixel ? v_mask : 0);
						break;
					case 2:
						// 0 clears pixel
						ssd1306_buffer[buffer_addr] &= input_pixel ? 0xFF : ~v_mask;
						break;
					case 3:
						// 1 sets pixel
						ssd1306_buffer[buffer_addr] |= input_pixel ? v_mask : 0;
						break;
					case 4:
						// 0 sets pixel
						ssd1306_buffer[buffer_addr] |= !input_pixel ? v_mask : 0;
						break;
					case 5:
						// 1 clears pixel
						ssd1306_buffer[buffer_addr] &= input_pixel ? ~v_mask : 0xFF;
						break;
				}
			}
			#if SSD1306_LOG_IMAGE == 1
			printf("%02x ", input_byte);
			#endif
		}
		#if SSD1306_LOG_IMAGE == 1
		printf("\n\r");
		#endif
	}
}

/*
 *  fast vert line
 */
void ssd1306_drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color)
{
	// clipping
	if((x >= SSD1306_W) || (y >= SSD1306_H)) return;
	if((y+h-1) >= SSD1306_H) h = SSD1306_H-y;
	while(h--)
	{
        ssd1306_drawPixel(x, y++, color);
	}
}

/*
 *  fast horiz line
 */
void ssd1306_drawFastHLine(uint32_t x, uint32_t y, uint32_t w, uint32_t color)
{
	// clipping
	if((x >= SSD1306_W) || (y >= SSD1306_H)) return;
	if((x+w-1) >= SSD1306_W)  w = SSD1306_W-x;

	while (w--)
	{
        ssd1306_drawPixel(x++, y, color);
	}
}

/*
 * abs() helper function for line drawing
 */
int gfx_abs(int x)
{
	return (x<0) ? -x : x;
}

/*
 * swap() helper function for line drawing
 */
void gfx_swap(int *z0, int *z1)
{
	uint16_t temp = *z0;
	*z0 = *z1;
	*z1 = temp;
}

/*
 * Bresenham line draw routine swiped from Wikipedia
 */
void ssd1306_drawLine(int x0, int y0, int x1, int y1, uint32_t color)
{
	int32_t steep;
	int32_t deltax, deltay, error, ystep, x, y;

	/* flip sense 45deg to keep error calc in range */
	steep = (gfx_abs(y1 - y0) > gfx_abs(x1 - x0));

	if(steep)
	{
		gfx_swap(&x0, &y0);
		gfx_swap(&x1, &y1);
	}

	/* run low->high */
	if(x0 > x1)
	{
		gfx_swap(&x0, &x1);
		gfx_swap(&y0, &y1);
	}

	/* set up loop initial conditions */
	deltax = x1 - x0;
	deltay = gfx_abs(y1 - y0);
	error = deltax/2;
	y = y0;
	if(y0 < y1)
		ystep = 1;
	else
		ystep = -1;

	/* loop x */
	for(x=x0;x<=x1;x++)
	{
		/* plot point */
		if(steep)
			/* flip point & plot */
			ssd1306_drawPixel(y, x, color);
		else
			/* just plot */
			ssd1306_drawPixel(x, y, color);

		/* update error */
		error = error - deltay;

		/* update y */
		if(error < 0)
		{
			y = y + ystep;
			error = error + deltax;
		}
	}
}

/*
 *  draws a circle
 */
void ssd1306_drawCircle(int x, int y, int radius, int color)
{
    /* Bresenham algorithm */
    int x_pos = -radius;
    int y_pos = 0;
    int err = 2 - 2 * radius;
    int e2;

    do {
        ssd1306_drawPixel(x - x_pos, y + y_pos, color);
        ssd1306_drawPixel(x + x_pos, y + y_pos, color);
        ssd1306_drawPixel(x + x_pos, y - y_pos, color);
        ssd1306_drawPixel(x - x_pos, y - y_pos, color);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos) {
              e2 = 0;
            }
        }
        if (e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while (x_pos <= 0);
}

/*
 *  draws a filled circle
 */
void ssd1306_fillCircle(int x, int y, int radius, int color)
{
    /* Bresenham algorithm */
    int x_pos = -radius;
    int y_pos = 0;
    int err = 2 - 2 * radius;
    int e2;

    do {
        ssd1306_drawPixel(x - x_pos, y + y_pos, color);
        ssd1306_drawPixel(x + x_pos, y + y_pos, color);
        ssd1306_drawPixel(x + x_pos, y - y_pos, color);
        ssd1306_drawPixel(x - x_pos, y - y_pos, color);
        ssd1306_drawFastHLine(x + x_pos, y + y_pos, 2 * (-x_pos) + 1, color);
        ssd1306_drawFastHLine(x + x_pos, y - y_pos, 2 * (-x_pos) + 1, color);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos) {
                e2 = 0;
            }
        }
        if(e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while(x_pos <= 0);
}

/*
 *  draw a rectangle
 */
void ssd1306_drawRect(int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color)
{
	ssd1306_drawFastVLine(x, y, h, color);
	ssd1306_drawFastVLine(x+w-1, y, h, color);
	ssd1306_drawFastHLine(x, y, w, color);
	ssd1306_drawFastHLine(x, y+h-1, w, color);
}

/*
 * fill a rectangle
 */
void ssd1306_fillRect(uint32_t x, uint32_t y, uint8_t w, uint32_t h, uint32_t color)
{
	uint32_t m, n=y, iw = w;
	
	/* scan vertical */
	while(h--)
	{
		m=x;
		w=iw;
		/* scan horizontal */
		while(w--)
		{
			/* invert pixels */
			ssd1306_drawPixel(m++, n, color);
		}
		n++;
	}
}

/*
 * invert a rectangle in the buffer
 */
void ssd1306_xorrect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	uint8_t m, n=y, iw = w;
	
	/* scan vertical */
	while(h--)
	{
		m=x;
		w=iw;
		/* scan horizontal */
		while(w--)
		{
			/* invert pixels */
			ssd1306_xorPixel(m++, n);
		}
		n++;
	}
}

/*
 * Draw character to the display buffer
 */
void ssd1306_drawchar(uint8_t x, uint8_t y, uint8_t chr, uint8_t color)
{
	uint16_t i, j, col;
	uint8_t d;
	
	for(i=0;i<8;i++)
	{
		d = fontdata[(chr<<3)+i];
		for(j=0;j<8;j++)
		{
			if(d&0x80)
				col = color;
			else
				col = (~color)&1;
			
			ssd1306_drawPixel(x+j, y+i, col);
			
			// next bit
			d <<= 1;
		}
	}
}

/*
 * draw a string to the display
 */
void ssd1306_drawstr(uint8_t x, uint8_t y, char *str, uint8_t color)
{
	uint8_t c;
	
	while((c=*str++))
	{
		ssd1306_drawchar(x, y, c, color);
		x += 8;
		if(x>120)
			break;
	}
}

/*
 * Draw character to the display buffer, scaled to size
 */
void ssd1306_drawchar_sz(uint8_t x, uint8_t y, uint8_t chr, uint8_t color, font_size_t font_size)
{
    uint16_t i, j, col;
    uint8_t d;

    // Determine the font scale factor based on the font_size parameter
    uint8_t font_scale = (uint8_t)font_size;

    // Loop through each row of the font data
    for (i = 0; i < 8; i++)
    {
        // Retrieve the font data for the current row
        d = fontdata[(chr << 3) + i];

        // Loop through each column of the font data
        for (j = 0; j < 8; j++)
        {
            // Determine the color to draw based on the current bit in the font data
            if (d & 0x80)
                col = color;
            else
                col = (~color) & 1;

            // Draw the pixel at the original size and scaled size using nested for-loops
            for (uint8_t k = 0; k < font_scale; k++) {
                for (uint8_t l = 0; l < font_scale; l++) {
                    ssd1306_drawPixel(x + (j * font_scale) + k, y + (i * font_scale) + l, col);
                }
            }

            // Move to the next bit in the font data
            d <<= 1;
        }
    }
}

/*
 * draw a string to the display buffer, scaled to size
 */
void ssd1306_drawstr_sz(uint8_t x, uint8_t y, char *str, uint8_t color, font_size_t font_size)
{
	uint8_t c;
	
	while((c=*str++))
	{
		ssd1306_drawchar_sz(x, y, c, color, font_size);
		x += 8 * font_size;
		if(x>128 - 8 * font_size)
			break;
	}
}

/*
 * initialize I2C and OLED
 */
uint8_t ssd1306_init(i2c_device_t *dev)
{
	// pulse reset
	ssd1306_rst();

	ssd1306_setbuf(0);
	
	// initialize OLED
#if !defined(SSD1306_CUSTOM_INIT_ARRAY) || !SSD1306_CUSTOM_INIT_ARRAY
	uint8_t *cmd_list = (uint8_t *)ssd1306_init_array;
	while(*cmd_list != SSD1306_TERMINATE_CMDS)
	{
		if(ssd1306_cmd(dev, *cmd_list++))
			return 1;
	}
	memcpy(display_cmdbuffer, cmd_array, sizeof(cmd_array)); 
	
	ssd1306_refresh(dev);	
#endif

	return 0;
}
