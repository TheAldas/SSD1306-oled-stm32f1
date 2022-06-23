/*
 * ssd1306.h
 *
 *
 */

#ifndef __I2C_OLED_H_
#define __I2C_OLED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "i2cs.h"
#include "ssd1306_print.h"

#define OLED_ADDRESS 0x3C
#define SCREEN_HEIGHT 32
#define SCREEN_WIDTH 128


#define SSD_COLOR_BLACK 0
#define SSD_COLOR_WHITE 1
#define SSD_COLOR_INVERSE 2

#define SSD1306_SUCCESS 0
#define SSD1306_ERROR_COMMUNICATION -1

  int ssd1306_init(void);

  void ssd1306_display_full(void);
  void ssd1306_display_empty(void);

  void set_contrast(uint8_t contrast_value);

  //display functions
  int ssd1306_display(void);
  void ssd1306_clear_display(void);
  void ssd1306_draw_pixel(int16_t x, int16_t y, uint8_t color);
  //draw functions
  void ssd1306_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color);
  void ssd1306_draw_h_line(int16_t x0, int16_t y0, int16_t x1, uint8_t color);
  void ssd1306_draw_v_line(int16_t x0, int16_t y0, int16_t y1, uint8_t color);
  void ssd1306_fill_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
  void ssd1306_fill_rect_round(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t cornerRadius, uint8_t color);
  void ssd1306_fill_circle(uint8_t midX, uint8_t midY, uint8_t radius, uint8_t color);
  void ssd1306_fill_circle_quarter(uint8_t midX, uint8_t midY, uint8_t radius, uint8_t quarter, uint8_t color);
  void ssd1306_draw_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
  void ssd1306_draw_rect_round(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t cornerRadius, uint8_t color);
  void ssd1306_draw_circle(uint8_t midX, uint8_t midY, uint8_t radius, uint8_t color);
  void ssd1306_draw_circle_quarter(uint8_t midX, uint8_t midY, uint8_t radius, uint8_t quarter, uint8_t color);
  void ssd1306_draw_XBM(const uint8_t *bitmap, uint8_t width, uint8_t height, uint8_t x0, uint8_t y0, uint8_t color);
  // Text functions
  void ssd1306_set_font(const unsigned char *fonts);
  int ssd1306_write(uint8_t c);
  void ssd1306_set_cursor(uint8_t column, uint8_t row);
  void ssd1306_set_cursor_coord(uint8_t coord_x, uint8_t coord_y);
  void ssd1306_set_cursor_column(uint8_t column);
  void ssd1306_set_cursor_row(uint8_t row);
  void ssd1306_advance_cursor_row(uint8_t row_count, uint8_t column);
  uint8_t ssd1306_get_font_height();
  uint16_t ssd1306_get_char_width(uint8_t c);
  uint16_t ssd1306_get_text_width(const char text[]);
  void ssd1306_set_text_offset(uint8_t offsetX, uint8_t offsetY) ;
  void ssd1306_set_text_scale(uint8_t textScale);
  void ssd1306_set_text_line_spacing(uint8_t lineSpacing);
  void ssd1306_set_text_letter_spacing(uint8_t letterSpacing);
  // Display command functions
  int sd1306_send_command(uint8_t command);
  int ssd1306_send_command_with_value(uint8_t command, uint8_t value);
  void ssd1306_set_contrast(uint8_t contrast_value);
  void ssd1306_set_display_on(uint8_t display_on);
  void ssd1306_invert_display(uint8_t invert);
  void ssd1306_flip_vertically(uint8_t flip);

  uint8_t ssd1306_get_screen_height();
  uint8_t ssd1306_get_screen_width();

#ifdef __cplusplus
}
#endif
#endif /* __SSD1306_H_*/
