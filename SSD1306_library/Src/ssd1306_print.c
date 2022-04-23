#include "ssd1306_print.h"

static size_t print_int(int n);
static size_t print_float(double n, uint16_t precision);
static size_t print_str(char str[]);

int ssd1306_write(char c);

static int _atoi(char *str, uint8_t size)
{
  int n = 0;
  int8_t sign = 1;
  if(*str == '-')
    {
      sign = -1;
      str++;
    }
  while(size--)
    {
      n *= 10;
      n += *str - '0';
      str++;
    }
  return n * sign;
}


size_t ssd1306_printf(char *str, ...)
{
  va_list args;
  va_start(args, str);
  size_t n = 0;

  uint16_t i = 0;
  while(str[i] != '\0')
    {
      if(str[i] != '%')
	{
	  ssd1306_write(str[i]);
	  n++;
	  i++;
	}
      else
	{
	  uint16_t temp = ++i;
	  uint8_t found_specifier = 0;
	  while(str[temp] != '\0' && !found_specifier)
	    {
	      switch(str[temp])
	      {
		case 'c':
		  ssd1306_write((char)va_arg(args, int));
		  found_specifier = 1;
		  n++;
		  break;
		case 'd':
		case 'i':
		  n += print_int(va_arg(args, int));
		  found_specifier = 1;
		  break;
		case 'f':
		  while(str[i++] != '.' && i != temp);
		  //couldn't find precision point
		  if(temp < i) n += print_float(va_arg(args, double), 2);
		  //found precision point
		  else  n += print_float(va_arg(args, double), _atoi(&str[i], (temp - i)));
		  found_specifier = 1;
		  break;
		case 's':
		  n += print_str(va_arg(args, char*));
		  found_specifier = 1;
		  break;
		default:
		  break;
	      }
	      temp++;
	    }
	  i = temp;
	}
    }
  va_end(args);
  return n;
}

static size_t print_int(int n)
{
  if(n < 0)
    {
      ssd1306_write('-');
      n = -n;
    }

  if(n == 0)
    {
      ssd1306_write('0');
      return 1;
    }

  size_t digits = 0;
  int reversed_n = 0;
  while(n != 0)
    {
      reversed_n *= 10;
      reversed_n += n % 10;
      n /= 10;
      digits++;
    }

  uint8_t temp = digits;
  while(reversed_n != 0)
    {
      ssd1306_write(('0' + (reversed_n%10)));
      reversed_n /= 10;
      temp--;
    }
  while(temp--) ssd1306_write('0');
  return digits;
}

static size_t print_float(double n, uint16_t precision)
{
  size_t digits = 0;
  if(n < 0)
    {
      ssd1306_write('-');
      n = -n;
    }
  digits += print_int((int)n);
  digits += ssd1306_write('.');
  while(precision--)
    {
      if(precision == 0) n += 0.05;
      n = (n - (int)n) * 10;
      digits += print_int((int)n);
    }
  return digits;
}

static size_t print_str(char *str)
{
  size_t digits = 0;
  while (*str != '\0')
    {
      ssd1306_write(*str++);
      digits++;
    }
  return digits;
}

