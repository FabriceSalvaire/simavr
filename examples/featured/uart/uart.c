/**************************************************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/**************************************************************************************************/

#include "avr_ioport.h"
#include "avr_uart.h"

#include "sim_avr.h"
#include "sim_elf.h"
#include "sim_gdb.h"
#include "sim_vcd_file.h"

/**************************************************************************************************/

struct output_buffer
{
  char *str;
  int currlen;
  int alloclen;
};

static void
init_output_buffer (struct output_buffer *buf)
{
  buf->str = malloc (128);
  buf->str[0] = 0;
  buf->currlen = 0;
  buf->alloclen = 128;
}

static void
uart_hook (struct avr_irq_t *irq, uint32_t value, void *param)
{
  struct output_buffer *buf = param;

  // reallocate if necessary and append the character passed via the value argument
  if (buf->currlen == buf->alloclen - 1)
    {
      buf->alloclen *= 2;
      buf->str = realloc (buf->str, buf->alloclen);
    }
  buf->str[buf->currlen++] = value;
  buf->str[buf->currlen] = 0;

  // echo on stdout
  // printf("<%c>", (char) value);
}

/**************************************************************************************************/

int
main (int argc, char *argv[])
{
  elf_firmware_t firmware;
  const char *firmware_path = "atmega88_uart_echo.axf";
  elf_read_firmware (firmware_path, &firmware);
  printf ("firmware %s f=%d mmcu=%s\n", firmware_path, (int) firmware.frequency, firmware.mmcu);

  avr_t *avr = avr_make_mcu_by_name (firmware.mmcu);
  if (!avr)
    {
      fprintf (stderr, "%s: AVR '%s' not known\n", argv[0], firmware.mmcu);
      exit (1);
    }
  avr_init (avr);
  avr_load_firmware (avr, &firmware);

  avr->log = LOG_TRACE;

  // Register hook to capture UART output
  char uart = '0';
  struct output_buffer uart_buffer;
  init_output_buffer (&uart_buffer);
  avr_irq_register_notify (avr_io_getirq (avr, AVR_IOCTL_UART_GETIRQ (uart), UART_IRQ_OUTPUT),
                           uart_hook, &uart_buffer);

  // run avr until it sleeps
  while (1)
    {
      int state = avr_run (avr);
      if (state == cpu_Sleeping)
        break;
    }
  avr_terminate (avr);

  printf("UART received: %s\n", uart_buffer.str);
  
  return EXIT_SUCCESS;
}

/***************************************************************************************************
 * 
 * End
 * 
 **************************************************************************************************/
