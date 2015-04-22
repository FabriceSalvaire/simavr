####################################################################################################

import os
import sys

from cffi import FFI

####################################################################################################

import ArduinoPinMapping

####################################################################################################

ffi = FFI()

module_path = os.path.dirname(os.path.realpath(__file__))

api_path = os.path.join(module_path, 'simavr-api.h')
with open(api_path, 'r') as f:
    simavr_api = f.read()
ffi.cdef(simavr_api)

source = """
#include "sim_avr.h"
#include "sim_cycle_timers.h"
#include "sim_elf.h"
#include "sim_irq.h"
#include "sim_time.h"
#include "sim_vcd_file.h"

#include "avr_ioport.h"

uint32_t
avr_ioctl_ioport_getirq(const char *port) {
  return AVR_IOCTL_IOPORT_GETIRQ(port[0]);
}

void
set_log_level(avr_t *avr, uint8_t level) {
  avr->log = level;
}

"""

simavr = ffi.verify(source,
                    include_dirs=[os.path.join(module_path, 'include')],
                    library_dirs=[module_path],
                    libraries=['simavr'])

####################################################################################################

def to_string(x):
    return str(x).encode('utf-8');

####################################################################################################

firmware = ffi.new('elf_firmware_t *')

firmware_path = '../../../build/firmwares/blink-led.elf'
rc = simavr.elf_read_firmware(to_string(firmware_path), firmware)
if rc == -1:
    sys.exit(1)
print("firmware %s f=%d mmcu=%s\n" % (firmware_path, firmware.frequency, firmware.mmcu))

firmware.frequency = 16000000
avr = simavr.avr_make_mcu_by_name(to_string('atmega2560')) # firmware.mmcu
if avr == ffi.NULL:
    print("AVR '%s' not known\n" % firmware.mmcu)
    sys.exit (1)
simavr.avr_init(avr)
simavr.avr_load_firmware(avr, firmware)

# avr.log = simavr.LOG_TRACE
simavr.set_log_level(avr, simavr.LOG_TRACE)

@ffi.callback("void(struct avr_irq_t *, uint32_t, void *)")
def pin_changed_hook(irq, value, param):
    print(value)

port, bit = ArduinoPinMapping.mega2560.to_port('PWM13')
simavr.avr_irq_register_notify(simavr.avr_io_getirq(avr, simavr.avr_ioctl_ioport_getirq(to_string(port)), bit),
                               pin_changed_hook, ffi.NULL)

while True:
    state = simavr.avr_run(avr)
    if state == simavr.cpu_Done or state == simavr.cpu_Crashed:
        break
simavr.avr_terminate(avr)
    
  # /*
  #  * VCD file initialization
  #  * 
  #  * This will allow you to create a "wave" file and display it in gtkwave Pressing "r" and "s"
  #  * during the demo will start and stop recording the pin changes
  #  */
  # avr_vcd_init (avr, "gtkwave_output.vcd", &vcd_file, 100000); // us
  # avr_vcd_add_signal (&vcd_file,
  #                     avr_io_getirq (avr, AVR_IOCTL_IOPORT_GETIRQ ('B'), IOPORT_IRQ_PIN_ALL),
  #                     8, // bits
  #                     "porth");

  # printf ("   Press 'q' to quit\n\n"
  #         "   Press 'r' to start recording a 'wave' file\n"
  #         "   Press 's' to stop recording\n");

  # // the AVR run on it's own thread. it even allows for debugging!
  # pthread_t run;
  # pthread_create (&run, NULL, avr_run_thread, NULL);

  # while (1)
  #   {
  #     switch (getchar())
  #       {
  #       case 'q':
  #         exit(0);
  #         break;
  #       case 'r':
  #         printf ("Starting VCD trace\n");
  #         avr_vcd_start (&vcd_file);
  #         break;
  #       case 's':
  #         printf ("Stopping VCD trace\n");
  #         avr_vcd_stop (&vcd_file);
  #         break;
  #       }
  #     fflush(stdin);
  #     fflush(stdout);
  #     // sleep (10); // s
  #   }

####################################################################################################
# 
# End
# 
####################################################################################################
