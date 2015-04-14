/*
 * ledramp.c
 * 
 * Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>
 * 
 * This file is part of simavr.
 * 
 * simavr is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * simavr is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with simavr.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "avr_ioport.h"
#include "sim_avr.h"
#include "sim_elf.h"
#include "sim_gdb.h"
#include "sim_vcd_file.h"

#include "button.h"

button_t button;
int do_button_press = 0;
avr_t *avr = NULL;
avr_vcd_t vcd_file;
uint8_t pin_state = 0; // current port B

void
display_led(void)
{
  char string[9];
  for (int i; i < 8; i++)
    string[i] = pin_state & (1 << i) ? '#' : ' ';
  string[8] = '\0';
  printf("%s\n", string);
}

/*
 * called when the AVR change any of the pins on port B so lets update
 * our buffer
 */
void
pin_changed_hook (struct avr_irq_t *irq, uint32_t value, void *param)
{
  pin_state = (pin_state & ~(1 << irq->irq)) | (value << irq->irq);
  display_led();
}

static void *
avr_run_thread (void *oaram)
{
  int previous_value = do_button_press;

  while (1)
    {
      avr_run (avr);
      if (do_button_press != previous_value)
        {
          previous_value = do_button_press;
          printf ("Button pressed\n");
          button_press (&button, 1000000);
        }
    }

  return NULL;
}

int
main (int argc, char *argv[])
{
  elf_firmware_t firmware;
  const char *firmware_path = "atmega48_ledramp.axf";
  elf_read_firmware (firmware_path, &firmware);
  printf ("firmware %s f=%d mmcu=%s\n", firmware_path, (int) firmware.frequency, firmware.mmcu);

  avr = avr_make_mcu_by_name (firmware.mmcu);
  if (!avr)
    {
      fprintf (stderr, "%s: AVR '%s' not known\n", argv[0], firmware.mmcu);
      exit (1);
    }
  avr_init (avr);
  avr_load_firmware (avr, &firmware);

  // initialize our 'peripheral'
  button_init (avr, &button, "button");
  // "connect" the output irq of the button to the pin 0 of the port C
  avr_connect_irq (button.irq + IRQ_BUTTON_OUT,
                   avr_io_getirq (avr, AVR_IOCTL_IOPORT_GETIRQ ('C'), 0));

  // connect all the pins on port B to our callback
  for (int i = 0; i < 8; i++)
    avr_irq_register_notify (avr_io_getirq (avr, AVR_IOCTL_IOPORT_GETIRQ ('B'), i),
                             pin_changed_hook, NULL);

  /*
   * VCD file initialization
   * 
   * This will allow you to create a "wave" file and display it in
   * gtkwave Pressing "r" and "s" during the demo will start and stop
   * recording the pin changes
   */
  avr_vcd_init (avr, "gtkwave_output.vcd", &vcd_file, 100000); // us
  avr_vcd_add_signal (&vcd_file,
                      avr_io_getirq (avr, AVR_IOCTL_IOPORT_GETIRQ ('B'), IOPORT_IRQ_PIN_ALL),
                      8, // bits
                      "portb");
  avr_vcd_add_signal (&vcd_file,
		      button.irq + IRQ_BUTTON_OUT,
		      1, // bits
                      "button");

  // 'raise' it, it's a "pullup"
  avr_raise_irq (button.irq + IRQ_BUTTON_OUT, 1);

  printf ("Demo launching: 'LED' bar is PORTB, updated every 1/64s by the AVR\n"
          "   firmware using a timer. If you press 'space' this presses a virtual\n"
          "   'button' that is hooked to the virtual PORTC pin 0 and will\n"
          "   trigger a 'pin change interrupt' in the AVR core, and will 'invert'\n"
          "   the display.\n"
          "   Press 'q' to quit\n\n"
          "   Press 'r' to start recording a 'wave' file\n"
	  "   Press 's' to stop recording\n");

  // the AVR run on it's own thread. it even allows for debugging!
  pthread_t run;
  pthread_create (&run, NULL, avr_run_thread, NULL);

  while (1)
    {
      switch (getchar())
	{
	case 'q':
	  exit(0);
	  break;
	case ' ':
	  do_button_press++; // pass the message to the AVR thread
	  break;
	case 'r':
	  printf ("Starting VCD trace\n");
	  avr_vcd_start (&vcd_file);
	  break;
	case 's':
	  printf ("Stopping VCD trace\n");
	  avr_vcd_stop (&vcd_file);
	  break;
	}
      fflush(stdin);
      fflush(stdout);
      // sleep (10); // s
    }
}
