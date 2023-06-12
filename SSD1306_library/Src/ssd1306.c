/*
 * i2c_oled.c
 */

#include <ssd1306.h>


//////////////////////////////////////////////////////
#define SSD_COMMAND_DISPLAY_OFF 0xAE
#define SSD_COMMAND_DISPLAY_ON 0xAF
#define SSD_COMMAND_MUX_RATIO 0xA8
#define SSD_COMMAND_DISPLAY_OFFSET 0xD3
#define SSD_COMMAND_SET_SEGMENT_RE_MAP 0xA0
#define SSD_COMMAND_SET_COM_OUTPUT_SCAN_DIRECTION_NORMAL 0xC0
#define SSD_COMMAND_SET_COM_OUTPUT_SCAN_DIRECTION_INVERSE 0xC8
#define SSD_COMMAND_COM_PINS_CONFIGURATION 0xDA //Set COM Pins hardware configuration
#define SSD_COMMAND_MEMORY_ADDRESSING_MODE 0x20//set addressing mode
#define SSD_COMMAND_CONTRAST 0x81//contrast between 0 and 255
#define SSD_COMMAND_DISABLE_ENTIRE_DISPLAY_ON 0xA4
#define SSD_COMMAND_ENABLE_ENTIRE_DISPLAY_ON 0xA5
#define SSD_COMMAND_SET_DISPLAY_NORMAL 0xA6
#define SSD_COMMAND_SET_DISPLAY_INVERSE 0xA7
#define SSD_COMMAND_SET_CLOCK_DIV 0xD5
#define SSD_COMMAND_CHARGE_PUMP 0x8D
#define SSD_COMMAND_PRE_CHARGE 0xD9 //set pre charge
#define SSD_COMMAND_DEACTIVATE_SCROLL 0x2E
#define SSD_COMMAND_SET_COLUMN_ADDRESS 0x21
#define SSD_COMMAND_SET_PAGE_ADDRESS 0x22

#define SSD_DISPLAY_FLIP_HORIZONTALLY 0x1



#define SSD_commandByte 0x00
#define SSD_dataByte 0x40

/////////////////////////////////////////////////////////


static int ssd1306_send_init_sequence(void);

/* Text settings local variables */
typedef struct
{
  int8_t line_spacing;
  int8_t letter_spacing;
  int8_t text_color;
  uint8_t text_scale;
  int16_t offset_x;
  int16_t offset_y;
} text_parameters_t;
static text_parameters_t text_parameters = {1, 1, SSD_COLOR_WHITE, 1, 0, 0};

typedef struct
{
  const unsigned char *font_family;
  uint16_t first_char_index;
  uint16_t last_char_index;
  uint8_t char_height;
} font_parameters_t;
static font_parameters_t font_parameters;

typedef struct
{
  uint8_t x;
  uint8_t y;
} cursor_coords_t;
cursor_coords_t cursor_coords = {0, 0};

static uint8_t screen_width = SCREEN_WIDTH;
static uint8_t screen_height = SCREEN_HEIGHT;

static int abs(int i);
static void swap_uint8_t(uint8_t *a, uint8_t *b);
static void swap_int16_t(int16_t *a, int16_t *b);

static uint8_t screen_buffer[(SCREEN_WIDTH * ((SCREEN_HEIGHT + 7) / 8))] = {0};
static uint8_t was_buffer_updated = 0;
static uint16_t updated_pixel_min_x = 255, updated_pixel_min_y = 255, updated_pixel_max_x = 0, updated_pixel_max_y = 0;

static const uint8_t addr_res_cmd_list[] = {
    SSD_commandByte,
    SSD_COMMAND_SET_PAGE_ADDRESS,
    0x00, SCREEN_HEIGHT - 1,
    SSD_COMMAND_SET_COLUMN_ADDRESS,
    0x00, (SCREEN_WIDTH - 1)
};



int ssd1306_init(void)
{
  return ssd1306_send_init_sequence();
}

//put pixel in buffer
void ssd1306_draw_pixel(int16_t x, int16_t y, uint8_t color)
{
  if(x >= screen_width || y >= screen_height || x < 0 || y < 0) return;
#ifdef USE_QUICK_DISPLAY
  was_buffer_updated = 1;
  if(updated_pixel_min_x > x) updated_pixel_min_x = x;
  if(updated_pixel_max_x < x) updated_pixel_max_x = x;
  if(updated_pixel_min_y > y) updated_pixel_min_y = y;
  if(updated_pixel_max_y < y) updated_pixel_max_y = y;
#endif
  switch (color) {
    case SSD_COLOR_BLACK:
      screen_buffer[((int)((y >> 3) * (screen_width)) + x)] &= ~(1 << (y & 0b111));
      break;
    case SSD_COLOR_WHITE:
      screen_buffer[((int)((y >> 3) * (screen_width)) + x)] |= (1 << (y & 0b111));
      break;
    default:
      screen_buffer[((int)((y >> 3) * (screen_width)) + x)] ^= (1 << (y & 0b111));
      break;
  }
}

//draw line function
void ssd1306_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color)
{
  x0 = (x0 < screen_width) ? x0 : (screen_width - 1);
  y0 = (y0 < screen_height) ? y0 : (screen_height - 1);
  x1 = (x1 < screen_width) ? x1 : (screen_width - 1);
  y1 = (y1 < screen_height) ? y1 : (screen_height - 1);
  int16_t slopeDirection = 1;
  uint8_t swapped = 0;

  if (y1 == y0)
    {
      ssd1306_draw_h_line(x0, y0, x1, color);
      return;
    }
  else if (x0 == x1)
    {
      ssd1306_draw_v_line(x0, y0, y1, color);
      return;
    }

  if (abs(x1 - x0) < abs(y1 - y0))
    {
      swapped = 1;
      swap_uint8_t(&x0, &y0);
      swap_uint8_t(&x1, &y1);
    }

  if (x0 > x1)
    {
      swap_uint8_t(&x0, &x1);
      swap_uint8_t(&y0, &y1);
    }

  if (y1 < y0) slopeDirection = -1;

  uint8_t dx = x1 - x0;
  uint8_t dy = abs(y1 - y0);

  int16_t err = dx >> 1;

  for (; x0 <= x1; x0++)
    {
      ssd1306_draw_pixel(swapped ? y0 : x0, swapped ? x0 : y0, color);
      err -= dy;
      if (err < 0)
	{
	  err += dx;
	  y0 += slopeDirection;
	}
    }
}

void ssd1306_draw_h_line(int16_t x0, int16_t y0, int16_t x1, uint8_t color)
{
  y0 = (y0 < screen_height) ? y0 : (screen_height - 1);
  x0 = (x0 < screen_width) ? x0 : (screen_width - 1);
  x1 = (x1 < screen_width) ? x1 : (screen_width - 1);
  y0 = (y0 >= 0) ? y0 : (0);
  x0 = (x0 >= 0) ? x0 : (0);
  x1 = (x1 >= 0) ? x1 : (0);

  if (x0 > x1) swap_int16_t(&x0, &x1);

  for (; x0 <= x1; x0++)
    {
      ssd1306_draw_pixel(x0, y0, color);
    }
}

void ssd1306_draw_v_line(int16_t x0, int16_t y0, int16_t y1, uint8_t color)
{
  y0 = (y0 < screen_height) ? y0 : (screen_height - 1);
  x0 = (x0 < screen_width) ? x0 : (screen_width - 1);
  y1 = (y1 < screen_height) ? y1 : (screen_height - 1);
  y0 = (y0 >= 0) ? y0 : (0);
  x0 = (x0 >= 0) ? x0 : (0);
  y1 = (y1 >= 0) ? y1 : (0);

  if (y0 > y1) swap_int16_t(&y0, &y1);

  for (; y0 <= y1; y0++)
    {
      ssd1306_draw_pixel(x0, y0, color);
    }
}

void ssd1306_fill_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
  width--;
  for(uint8_t i = 0; i < height; i++)
    {
      ssd1306_draw_h_line(x, y + i, x + width, color);
    }
}

void ssd1306_fill_rect_round(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t cornerRadius, uint8_t color)
{
  if(width < 1 || height < 1) return;
  width--;
  height--;

  ssd1306_fill_circle_quarter(x + width - cornerRadius, y + cornerRadius, cornerRadius, 0, color);
  ssd1306_fill_circle_quarter(x + cornerRadius, y + cornerRadius, cornerRadius, 1, color);
  ssd1306_fill_circle_quarter(x + cornerRadius, y + height - cornerRadius, cornerRadius, 2, color);
  ssd1306_fill_circle_quarter(x + width - cornerRadius, y + height - cornerRadius, cornerRadius, 3, color);

  for(uint8_t i = 0; i < height + 1; i++)
    {
      if(cornerRadius >= i || height - cornerRadius <= i) ssd1306_draw_h_line(x + cornerRadius + 1 , y + i , x + width- cornerRadius - 1, color);
      else ssd1306_draw_h_line(x, y + i, x + width, color);
    }
}

void ssd1306_fill_circle(uint8_t midX, uint8_t midY, uint8_t radius, uint8_t color)
{
  uint32_t x = radius, y = 0, radiusThreshold = radius * radius + radius;
  ssd1306_draw_h_line(midX - x, midY, midX + x, color);

  while (x > y) {
      y++;

      if (radiusThreshold < (x * x + y * y)) {

	  ssd1306_draw_h_line(midX - y + 1, midY - x, midX + y - 1, color);
	  ssd1306_draw_h_line(midX - y + 1, midY + x, midX + y - 1, color);

	  x--;
      }
      if(x < y) break;

      ssd1306_draw_h_line(midX - x, midY + y, midX + x, color);
      ssd1306_draw_h_line(midX - x, midY - y, midX + x, color);
  }
}

void ssd1306_fill_circle_quarter(uint8_t midX, uint8_t midY, uint8_t radius, uint8_t quarter, uint8_t color)
{
  uint32_t x = radius, y = 0, radiusThreshold = radius * radius + radius;
  if(quarter == 0 || quarter == 3) ssd1306_draw_h_line(midX, midY, midX + x, color);
  else ssd1306_draw_h_line(midX - x, midY, midX, color);

  while (x > y) {
      y++;

      if (radiusThreshold < (x * x + y * y)) {
	  switch (quarter)
	  {
	    case 0:
	      ssd1306_draw_h_line(midX, midY - x, midX + y - 1, color);
	      break;
	    case 1:
	      ssd1306_draw_h_line(midX - y + 1, midY - x, midX, color);
	      break;
	    case 2:
	      ssd1306_draw_h_line(midX - y + 1, midY + x, midX, color);
	      break;
	    case 3:
	      ssd1306_draw_h_line(midX, midY + x, midX + y - 1, color);
	      break;
	    default:
	      break;
	  }
	  x--;
      }
      if(x < y) break;

      switch (quarter)
      {
	case 0:
	  ssd1306_draw_h_line(midX, midY - y, midX + x, color);
	  break;
	case 1:
	  ssd1306_draw_h_line(midX - x, midY - y, midX, color);
	  break;
	case 2:
	  ssd1306_draw_h_line(midX - x, midY + y, midX, color);
	  break;
	case 3:
	  ssd1306_draw_h_line(midX, midY + y, midX + x, color);
	  break;
	default:
	  break;
      }
  }
}

void ssd1306_draw_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
  ssd1306_draw_h_line(x, y, x + width - 1, color);
  ssd1306_draw_v_line(x + width - 1, y + 1, y + height - 1, color);
  ssd1306_draw_h_line(x, y + height - 1, x + width - 2, color);
  ssd1306_draw_v_line(x, y + 1, y + height - 2, color);
}

void ssd1306_draw_rect_round(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t cornerRadius, uint8_t color)
{
  if(width < 1 || height < 1) return;
  width--;
  height--;

  ssd1306_draw_circle_quarter(x + width - cornerRadius, y + cornerRadius, cornerRadius, 0, color);
  ssd1306_draw_circle_quarter(x + cornerRadius, y + cornerRadius, cornerRadius, 1, color);
  ssd1306_draw_circle_quarter(x + cornerRadius, y + height - cornerRadius, cornerRadius, 2, color);
  ssd1306_draw_circle_quarter(x + width - cornerRadius, y + height - cornerRadius, cornerRadius, 3, color);

  ssd1306_draw_h_line(x + cornerRadius + 1, y, x + width - cornerRadius, color);
  ssd1306_draw_v_line(x + width, y  + cornerRadius + 1, y + height - cornerRadius - 1, color);
  ssd1306_draw_h_line(x + cornerRadius + 1, y + height, x + width - cornerRadius, color);
  ssd1306_draw_v_line(x, y + cornerRadius + 1, y + height  - cornerRadius - 1, color);
}

void ssd1306_draw_circle(uint8_t midX, uint8_t midY, uint8_t radius, uint8_t color)
{
  //calculating only one quarter of the circle until x is y
  //decreasing x every time if x^2 + y^2 > r^2 + r
  //r^2 + r will be a 'radiusThreshold' for now, I don't know how to call it properly, cause I've come up with the formula
  //it might be similar to mid point circle drawing algorithm
  uint32_t x = radius, y = 0, radiusThreshold = radius * radius + radius;
  ssd1306_draw_pixel(midX + radius, midY, color);
  if (radius != 0) {
      ssd1306_draw_pixel(midX, midY + radius, color);
      ssd1306_draw_pixel(midX, midY - radius, color);
      ssd1306_draw_pixel(midX - radius, midY, color);
  }

  while (x > y)
    {
      y++;

      if (radiusThreshold < (x * x + y * y))
	{
	  x--;
	}
      if(x < y) break;

      ssd1306_draw_pixel(midX + x, midY + y, color);
      ssd1306_draw_pixel(midX + x, midY - y, color);
      ssd1306_draw_pixel(midX - x, midY + y, color);
      ssd1306_draw_pixel(midX - x, midY - y, color);
      if (x != y)
	{
	  ssd1306_draw_pixel(midX + y, midY + x, color);
	  ssd1306_draw_pixel(midX + y, midY - x, color);
	  ssd1306_draw_pixel(midX - y, midY + x, color);
	  ssd1306_draw_pixel(midX - y, midY - x, color);
	}
    }
}

/*circle quarters:
 |---|---|
 | 1 | 0 |
 |---|---|
 | 2 | 3 |
 |---|---|
 */
void ssd1306_draw_circle_quarter(uint8_t midX, uint8_t midY, uint8_t radius, uint8_t quarter, uint8_t color)
{
  uint32_t x = radius, y = 0, radiusThreshold = radius * radius + radius;

  if (radius != 0) {
      switch (quarter)
      {
	case 0:
	  ssd1306_draw_pixel(midX + radius, midY, color);
	  ssd1306_draw_pixel(midX, midY - radius, color);
	  break;
	case 1:
	  ssd1306_draw_pixel(midX, midY - radius, color);
	  ssd1306_draw_pixel(midX - radius, midY, color);
	  break;
	case 2:
	  ssd1306_draw_pixel(midX - radius, midY, color);
	  ssd1306_draw_pixel(midX, midY + radius, color);
	  break;
	case 3:
	  ssd1306_draw_pixel(midX, midY + radius, color);
	  ssd1306_draw_pixel(midX + radius, midY, color);
	  break;
	default:
	  break;
      }
  }else{
      ssd1306_draw_pixel(midX, midY, color);
  }
  while (x > y) {
      y++;

      if (radiusThreshold < (x * x + y * y)) {
	  x--;
      }

      if(x < y) break;

      switch (quarter)
      {
	case 0:
	  ssd1306_draw_pixel(midX + x, midY - y, color);
	  if (x != y) ssd1306_draw_pixel(midX + y, midY - x, color);
	  break;
	case 1:
	  ssd1306_draw_pixel(midX - x, midY - y, color);
	  if (x != y) ssd1306_draw_pixel(midX - y, midY - x, color);
	  break;
	case 2:
	  ssd1306_draw_pixel(midX - x, midY + y, color);
	  if (x != y)ssd1306_draw_pixel(midX - y, midY + x, color);
	  break;
	case 3:
	  ssd1306_draw_pixel(midX + x, midY + y, color);
	  if (x != y)ssd1306_draw_pixel(midX + y, midY + x, color);
	  break;
	default:
	  break;
      }
  }
}

void ssd1306_draw_XBM(const uint8_t *bitmap, uint8_t width, uint8_t height, uint8_t x0, uint8_t y0, uint8_t color)
{
  height = height + y0 <= screen_height ?  height : screen_height - y0;
  uint8_t bmpByte, widthInBytes = (width + 7) >> 3;
  for(uint8_t y = 0; y < height; y++)
    {
      for(uint8_t x = 0; x < width; x++)
	{
	  if((x & 0b111) == 0) bmpByte = (bitmap[y * widthInBytes + (x >> 3)]);
	  if((bmpByte >> (x & 0b111)) & 1) ssd1306_draw_pixel(x0 + x, y0 + y, color);
	}
    }
}

/////////////////// TEXT /////////////////

//Used format for fonts: http://ww1.microchip.com/downloads/en/AppNotes/01182b.pdf
void ssd1306_set_font(const unsigned char *fonts)
{
  font_parameters.font_family = fonts;
  font_parameters.first_char_index = (font_parameters.font_family[0x03]) << 8 | (font_parameters.font_family[0x02]);
  font_parameters.last_char_index = (font_parameters.font_family[0x05]) << 8 | (font_parameters.font_family[0x04]);
  font_parameters.char_height = (font_parameters.font_family[0x06]);
}

int ssd1306_write(uint8_t c)
{
  uint8_t charBitmapByte, bitsLeft;
  if(c == '\n'){ //transfer to new line
      cursor_coords.x = 0;
      cursor_coords.y += font_parameters.char_height * text_parameters.text_scale + text_parameters.line_spacing;
      return 1;
  }
  else if(c == '\r') return 1; //ignoring carriage return
  else if (c == ' ')
    {
      cursor_coords.x += text_parameters.text_scale + text_parameters.line_spacing;
      return 1;
    }
  else if(c < font_parameters.first_char_index || c > font_parameters.last_char_index) return 1;
  uint16_t charHeadIndex =  (((int)c - font_parameters.first_char_index) << 2) + 8 ;
  uint8_t charWidth = (font_parameters.font_family[charHeadIndex]);
  uint32_t charOffset =
      (((uint32_t)(font_parameters.font_family[charHeadIndex + 3])) << 16)
      | (((uint16_t)(font_parameters.font_family[charHeadIndex + 2])) << 8)
      | (font_parameters.font_family[charHeadIndex+1]);
  for(uint8_t clmnByte = 0; clmnByte < ((charWidth + 7) >> 3); clmnByte++)
    {
      for(uint8_t y = 0; y < font_parameters.char_height; y++)
	{
	  charBitmapByte = (font_parameters.font_family[charOffset + y * ((charWidth + 7) >> 3) + clmnByte]);
	  bitsLeft = charWidth < (1 + clmnByte) << 3 ? (charWidth & 0b111) : 8;
	  for(uint8_t x = 0; x < bitsLeft; x++)
	    {
	      if((charBitmapByte >> x) & 1)
		for(uint8_t sX = 0; sX < text_parameters.text_scale; sX++)
		  for(uint8_t sY = 0; sY < text_parameters.text_scale; sY++)
		    ssd1306_draw_pixel(text_parameters.offset_x + cursor_coords.x + (clmnByte << 3) + x * text_parameters.text_scale + sX,
				       text_parameters.offset_y + cursor_coords.y + y * text_parameters.text_scale + sY,
				       text_parameters.text_color);
	    }
	}
    }
  cursor_coords.x += charWidth * text_parameters.text_scale + text_parameters.letter_spacing;
  return 1;
}

void ssd1306_set_cursor(uint8_t column, uint8_t row)
{
  cursor_coords.x = column;
  cursor_coords.y = (font_parameters.char_height * text_parameters.text_scale * row) + (text_parameters.line_spacing * row);
}

void ssd1306_set_cursor_coord(uint8_t coord_x, uint8_t coord_y)
{
  cursor_coords.x = coord_x;
  cursor_coords.y = coord_y;
}

void ssd1306_advance_cursor_row(uint8_t row_count, uint8_t column)
{
  cursor_coords.y += (font_parameters.char_height * text_parameters.text_scale * row_count) + (text_parameters.line_spacing * row_count);
  cursor_coords.x = column;
}

uint8_t ssd1306_get_font_height()
{
  return font_parameters.char_height * text_parameters.text_scale;
}

uint16_t ssd1306_get_char_width(uint8_t c)
{
  if (c == ' ')
    {
      return text_parameters.text_scale + text_parameters.letter_spacing;
    }
  else if(c == '\n'
      || c == '\r'
	  || c < font_parameters.first_char_index
	  || c > font_parameters.last_char_index) return 0;
  uint16_t charHeadIndex =  (((int)c - font_parameters.first_char_index) << 2) + 8 ;
  uint8_t charWidth = (font_parameters.font_family[charHeadIndex]);

  cursor_coords.x += charWidth * text_parameters.text_scale + text_parameters.letter_spacing;
  return 1;
}

uint16_t ssd1306_get_text_width(const char text[])
{
  uint16_t most_width = 0, current_width = 0;
  while(*text != '\0')
    {
      if (*text == ' ')
	{
	  current_width += text_parameters.text_scale + text_parameters.letter_spacing;
	}
      else if(*text == '\n')
	{
	  if(most_width < current_width) most_width = current_width;
	  current_width = 0;
	}
      else if(*text >= font_parameters.first_char_index
	  && *text <= font_parameters.last_char_index)
	{
	  uint16_t charHeadIndex =  ((*text - font_parameters.first_char_index) << 2) + 8 ;
	  uint8_t charWidth = (font_parameters.font_family[charHeadIndex]);
	  current_width += charWidth * text_parameters.text_scale + text_parameters.letter_spacing;
	}
      text++;
    }
  if(most_width < current_width) most_width = current_width;
  return most_width;
}

void ssd1306_set_text_offset(uint8_t offset_x, uint8_t offset_y)
{
  text_parameters.offset_x = offset_x;
  text_parameters.offset_y = offset_y;
}

void ssd1306_set_text_scale(uint8_t text_scale)
{
  text_parameters.text_scale = text_scale;
}
void ssd1306_set_text_line_spacing(uint8_t line_spacing)
{
  text_parameters.line_spacing = line_spacing;
}
void ssd1306_set_text_letter_spacing(uint8_t letter_spacing)
{
  text_parameters.letter_spacing = letter_spacing;
}
void ssd1306_set_cursor_column(uint8_t column)
{
  cursor_coords.x = column;
}

void ssd1306_set_cursor_row(uint8_t row)
{
  cursor_coords.y = (font_parameters.char_height * text_parameters.text_scale * row) + (text_parameters.line_spacing * row);
}

///////////////// TEXT END ////////////////

/////////////// DISPLAY COMMANDS ///////////////

int ssd1306_send_command(uint8_t command)
{
  i2cs_start_transmission(OLED_ADDRESS, 0);
  i2cs_send_byte(SSD_commandByte);
  i2cs_send_byte(command);
  i2cs_end_transmission();
  return SSD1306_SUCCESS;
}

int ssd1306_send_command_with_value(uint8_t command, uint8_t value)
{
  i2cs_start_transmission(OLED_ADDRESS, 0);
  i2cs_send_byte(SSD_commandByte);
  i2cs_send_byte(command);
  i2cs_send_byte(value);
  i2cs_end_transmission();
  return SSD1306_SUCCESS;
}

void ssd1306_set_contrast(uint8_t contrast_value)
{
  ssd1306_send_command_with_value(SSD_COMMAND_CONTRAST, contrast_value);
}

void ssd1306_set_display_on(uint8_t display_on)
{
  if(display_on) ssd1306_send_command(SSD_COMMAND_DISPLAY_ON);
  else ssd1306_send_command(SSD_COMMAND_DISPLAY_OFF);
}

void ssd1306_invert_display(uint8_t invert)
{
  if(invert) ssd1306_send_command(SSD_COMMAND_SET_DISPLAY_INVERSE);
  else ssd1306_send_command(SSD_COMMAND_SET_DISPLAY_NORMAL);
}

void ssd1306_flip_vertically(uint8_t flip)
{
  if(flip)
    {
      ssd1306_send_command(SSD_COMMAND_SET_COM_OUTPUT_SCAN_DIRECTION_INVERSE);
    }else{
	ssd1306_send_command(SSD_COMMAND_SET_COM_OUTPUT_SCAN_DIRECTION_NORMAL);
    }
}

uint8_t ssd1306_get_screen_height() {return screen_height;}
uint8_t ssd1306_get_screen_width() {return screen_width;}

///////////// DISPLAY COMMANDS END //////////////////

int ssd1306_display(void)
{
#ifdef USE_QUICK_DISPLAY
  if(!was_buffer_updated) return SSD1306_SUCCESS;
  uint8_t total_updated_pages = (updated_pixel_max_y >> 3) - (updated_pixel_min_y >> 3) + 1;
  uint8_t *ptr;
  uint8_t data_frame[] = {
      SSD_commandByte,
      SSD_COMMAND_SET_PAGE_ADDRESS,
      (updated_pixel_min_y >> 3), (updated_pixel_max_y >> 3),
      SSD_COMMAND_SET_COLUMN_ADDRESS,
      (updated_pixel_min_x), (updated_pixel_max_x)
  };

  if(i2cs_start_transmission(OLED_ADDRESS, 0) != I2C_SUCCESS) return SSD1306_ERROR_COMMUNICATION;
  if(i2cs_send_byte_array(data_frame, sizeof(data_frame)) != I2C_SUCCESS) return SSD1306_ERROR_COMMUNICATION;
  if(i2cs_end_transmission() != I2C_SUCCESS) return SSD1306_ERROR_COMMUNICATION;

  uint16_t columns_count = (updated_pixel_max_x - updated_pixel_min_x);

  if(i2cs_start_transmission(OLED_ADDRESS, 0) != I2C_SUCCESS) return SSD1306_ERROR_COMMUNICATION;
  if(i2cs_send_byte(SSD_dataByte) != I2C_SUCCESS) return SSD1306_ERROR_COMMUNICATION;
  for(uint8_t pages_count = 0; pages_count < total_updated_pages; pages_count++)
    {
      //      columns_count = 1;
      ptr = (screen_buffer+(SCREEN_WIDTH*((updated_pixel_min_y >> 3) + pages_count))+updated_pixel_min_x);
      columns_count = (updated_pixel_max_x - updated_pixel_min_x+1);
      while (columns_count--) {
	  if(i2cs_send_byte(*ptr++) != I2C_SUCCESS) break;
      }
    }
  if(i2cs_end_transmission() != I2C_SUCCESS) return SSD1306_ERROR_COMMUNICATION;
  updated_pixel_min_x = 255;
  updated_pixel_min_y = 255;
  updated_pixel_max_x = 0;
  updated_pixel_max_y = 0;
  was_buffer_updated = 0;

#else
  if(i2cs_start_transmission(OLED_ADDRESS, 0) != I2C_SUCCESS) return SSD1306_ERROR_COMMUNICATION;
  if(i2cs_send_byte_array(addr_res_cmd_list, sizeof(addr_res_cmd_list)) != I2C_SUCCESS) return SSD1306_ERROR_COMMUNICATION;
  if(i2cs_end_transmission() != I2C_SUCCESS) return SSD1306_ERROR_COMMUNICATION;

  uint16_t columns_count = (screen_width * ((screen_height + 7) / 8));
  uint8_t *ptr = screen_buffer;

  if(i2cs_start_transmission(OLED_ADDRESS, 0) != I2C_SUCCESS) return SSD1306_ERROR_COMMUNICATION;
  if(i2cs_send_byte(SSD_dataByte) != I2C_SUCCESS) return SSD1306_ERROR_COMMUNICATION;
  while (columns_count--) {
      if(i2cs_send_byte(*ptr++) != I2C_SUCCESS) break;
  }
  if(i2cs_end_transmission() != I2C_SUCCESS) return SSD1306_ERROR_COMMUNICATION;
#endif
  return SSD1306_SUCCESS;
}

static void ssd1306_fill_display(uint8_t color)
{
  uint16_t columns_count = screen_width * ((screen_height + 7) / 8);
  uint8_t *buff_ptr = screen_buffer;
  switch(color)
  {
    case SSD_COLOR_BLACK:
      while(columns_count--) *buff_ptr++ = 0x00;
      break;
    case SSD_COLOR_WHITE:
      while(columns_count--) *buff_ptr++ = 0xFF;
      break;
    case SSD_COLOR_INVERSE:
      while(columns_count--)
	{
	  *buff_ptr = ~(*buff_ptr);
	  buff_ptr++;
	}
      break;
    default:
      break;
  }
#ifdef USE_QUICK_DISPLAY
  updated_pixel_min_x = 0;
  updated_pixel_min_y = 0;
  updated_pixel_max_x = SCREEN_WIDTH-1;
  updated_pixel_max_y = SCREEN_HEIGHT-1;
  was_buffer_updated = 1;
#endif
}

void ssd1306_clear_display(void)
{
  ssd1306_fill_display(SSD_COLOR_BLACK);
}

void ssd1306_display_empty(void)
{
  ssd1306_fill_display(SSD_COLOR_BLACK);
  ssd1306_display();
}

void ssd1306_display_full(void)
{
  ssd1306_fill_display(SSD_COLOR_WHITE);
  ssd1306_display();
}


static int ssd1306_send_init_sequence(void)
{
  uint8_t comPinsConf = 0x02;
  if(screen_width == 128 && screen_height == 64) comPinsConf = 0x12;
  uint8_t initList[] = {
      SSD_commandByte,
      SSD_COMMAND_DISPLAY_OFF,
      SSD_COMMAND_MUX_RATIO,
      (screen_height - 1),
      SSD_COMMAND_SET_PAGE_ADDRESS,
      0, (screen_height / 8 - 1),
      SSD_COMMAND_SET_COLUMN_ADDRESS,
      0, (screen_width - 1),
      SSD_COMMAND_DISPLAY_OFFSET,
      (0x00),
      (0x40), //set display start line to 0
      SSD_COMMAND_SET_SEGMENT_RE_MAP | SSD_DISPLAY_FLIP_HORIZONTALLY,
      SSD_COMMAND_SET_COM_OUTPUT_SCAN_DIRECTION_INVERSE,
      SSD_COMMAND_COM_PINS_CONFIGURATION,
      comPinsConf,
      SSD_COMMAND_MEMORY_ADDRESSING_MODE,
      0x00,
      SSD_COMMAND_CONTRAST,
      0xF7,
      SSD_COMMAND_DISABLE_ENTIRE_DISPLAY_ON,
      SSD_COMMAND_SET_DISPLAY_NORMAL,
      SSD_COMMAND_SET_CLOCK_DIV,
      0x80,
      SSD_COMMAND_CHARGE_PUMP,
      0x14,
      SSD_COMMAND_PRE_CHARGE,
      0x22,
      SSD_COMMAND_DEACTIVATE_SCROLL,
      SSD_COMMAND_DISPLAY_ON
  };
  if (i2cs_start_transmission(OLED_ADDRESS, 0) != I2C_SUCCESS) return SSD1306_ERROR_COMMUNICATION;
  i2cs_send_byte_array(initList, sizeof(initList));
  i2cs_end_transmission();
  //clearDisplay();
  ssd1306_display_empty();
  return SSD1306_SUCCESS;
}



static int abs(int i)
{
  return i > 0 ? i : -i;
}

static void swap_uint8_t(uint8_t *a, uint8_t *b)
{
  uint8_t temp = *a;
  *a = *b;
  *b = temp;
}

static void swap_int16_t(int16_t *a, int16_t *b)
{
  *a += *b;
  *b = *a - *b;
  *a -= *b;
}
