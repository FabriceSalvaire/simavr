#ifndef __POTENTIOMETER_H___
#define __POTENTIOMETER_H___

#include "sim_irq.h"

enum
  {
    IRQ_POTENTIOMETER_ADC_TRIGGER_IN = 0,
    IRQ_POTENTIOMETER_ADC_VALUE_OUT,
    IRQ_POTENTIOMETER_TEMP_VALUE_OUT,
    IRQ_POTENTIOMETER_TEMP_VALUE_IN,
    IRQ_POTENTIOMETER_COUNT
  };

typedef struct potentiometer_t
{
  avr_irq_t *irq;   // irq list
  struct avr_t *avr;   // keep it around so we can pause it
  unsigned int adc_mux_number;

  float value; // % of VCC
} potentiometer_t, *potentiometer_p;

void potentiometer_init (struct avr_t *avr,  potentiometer_p t, unsigned int adc_mux_number, float initial_value);
void potentiometer_set (potentiometer_p t, float value);

#endif /* __POTENTIOMETER_H___ */
