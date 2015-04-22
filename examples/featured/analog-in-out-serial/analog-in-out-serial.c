#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "avr_ioport.h"
#include "sim_avr.h"
#include "sim_elf.h"
#include "sim_gdb.h"
#include "sim_vcd_file.h"

#include "potentiometer.h"

potentiometer_t potentiometer;
avr_t *avr = NULL;
avr_vcd_t vcd_file;

/*
 * called when the AVR change any of the pins on port B
 */
void
pin_changed_hook (struct avr_irq_t *irq, uint32_t value, void *param)
{
  printf("%u\n", value);
}

static void *
avr_run_thread (void *oaram)
{
  while (1)
    {
      int state = avr_run (avr);
      if (state == cpu_Done || state == cpu_Crashed)
        break;
    }
  avr_terminate (avr);
  
  return NULL;
}

int
main (int argc, char *argv[])
{
  elf_firmware_t firmware;
  const char *firmware_path = "../../../firmwares/analog-in-out-serial.elf";
  int rc = elf_read_firmware (firmware_path, &firmware);
  if (rc == -1)
    exit (1);
  printf ("firmware %s f=%d mmcu=%s\n", firmware_path, (int) firmware.frequency, firmware.mmcu);

  firmware.frequency = 16e6;
  avr = avr_make_mcu_by_name ("atmega2560"); // firmware.mmcu
  if (!avr)
    {
      fprintf (stderr, "%s: AVR '%s' not known\n", argv[0], firmware.mmcu);
      exit (1);
    }
  avr_init (avr);
  avr_load_firmware (avr, &firmware);
  avr->aref = avr->avcc = avr->vcc = 5 * 1000;   // needed for ADC
  
  // Fixme: UART output ?
  avr->log = LOG_TRACE;

  potentiometer_init (avr, &potentiometer, 0, .5);
  
  // connect all the pins on port B to our callback
  // Fixme: PH6 pin 9
  // char port = 'H';
  char port = 'B';
  for (int i = 0; i < 8; i++)
    avr_irq_register_notify (avr_io_getirq (avr, AVR_IOCTL_IOPORT_GETIRQ (port), i),
                             pin_changed_hook, NULL);

  /*
   * VCD file initialization
   * 
   * This will allow you to create a "wave" file and display it in gtkwave Pressing "r" and "s"
   * during the demo will start and stop recording the pin changes
   */
  avr_vcd_init (avr, "gtkwave_output.vcd", &vcd_file, 100000); // us
  avr_vcd_add_signal (&vcd_file,
                      avr_io_getirq (avr, AVR_IOCTL_IOPORT_GETIRQ (port), IOPORT_IRQ_PIN_ALL),
                      8, // bits
                      "porth");

  printf ("   Press 'q' to quit\n\n"
          "   Press 'r' to start recording a 'wave' file\n"
	  "   Press 's' to stop recording\n"
	  "   Press '+/-' to change potentiometer\n"
	  );

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
	case 'r':
	  printf ("Starting VCD trace\n");
	  avr_vcd_start (&vcd_file);
	  break;
	case 's':
	  printf ("Stopping VCD trace\n");
	  avr_vcd_stop (&vcd_file);
	  break;
	case '+':
	  printf ("increase potentiometer\n");
	  potentiometer_set(&potentiometer, potentiometer.value + .1);
	  break;
	case '-':
	  printf ("decrease potentiometer\n");
	  potentiometer_set(&potentiometer, potentiometer.value - .1);
	  break;
	}
      fflush(stdin);
      fflush(stdout);
      // sleep (10); // s
    }
}
