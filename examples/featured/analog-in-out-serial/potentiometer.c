#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "avr_adc.h"

#include "potentiometer.h"

/*
 * called when the ADC could use a new value
 * The value returned is NOT in "ADC" mode, it's in millivolts
 */
static void
potentiometer_in_hook (struct avr_irq_t *irq, uint32_t value, void *param)
{
  potentiometer_p p = (potentiometer_p) param;
  avr_adc_mux_t v = *((avr_adc_mux_t *) & value);

  printf("%s (%2d/%2d)\n", __func__, p->adc_mux_number, v.src);

  if (v.src == p->adc_mux_number)
    {
      uint32_t int_value = p->value * 1024;
      printf("Send %u\n", int_value);
      avr_raise_irq (p->irq + IRQ_POTENTIOMETER_ADC_VALUE_OUT, int_value);
    }
}

static void
potentiometer_value_in_hook (struct avr_irq_t *irq, uint32_t value, void *param)
{
  potentiometer_p p = (potentiometer_p) param;
  float float_value = ((float) value) / 1024;
  p->value = float_value;
  avr_raise_irq (p->irq + IRQ_POTENTIOMETER_TEMP_VALUE_OUT, value);
}

static const char *irq_names[IRQ_POTENTIOMETER_COUNT] = {
  [IRQ_POTENTIOMETER_ADC_TRIGGER_IN] = "8<potentiometer.trigger",
  [IRQ_POTENTIOMETER_TEMP_VALUE_OUT] = "16>potentiometer.out",
  [IRQ_POTENTIOMETER_TEMP_VALUE_IN] = "16<potentiometer.in",
};

void
potentiometer_init (struct avr_t *avr, potentiometer_p p, uint adc_mux_number, float initial_value)
{
  p->avr = avr;
  p->irq = avr_alloc_irq (&avr->irq_pool, 0, IRQ_POTENTIOMETER_COUNT, irq_names);
  avr_irq_register_notify (p->irq + IRQ_POTENTIOMETER_ADC_TRIGGER_IN, potentiometer_in_hook, p);
  avr_irq_register_notify (p->irq + IRQ_POTENTIOMETER_TEMP_VALUE_IN, potentiometer_value_in_hook, p);

  p->adc_mux_number = adc_mux_number;
  p->value = initial_value;

  avr_irq_t *src = avr_io_getirq (p->avr, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_OUT_TRIGGER);
  avr_irq_t *dst = avr_io_getirq (p->avr, AVR_IOCTL_ADC_GETIRQ, adc_mux_number);
  if (src && dst)
    {
      avr_connect_irq (src, p->irq + IRQ_POTENTIOMETER_ADC_TRIGGER_IN);
      avr_connect_irq (p->irq + IRQ_POTENTIOMETER_ADC_VALUE_OUT, dst);
    }

  printf ("%s on ADC %d start %.2f\n", __func__, adc_mux_number, p->value);
}

void
potentiometer_set (potentiometer_p p, float value)
{
  if (value < 0 )
    value = .0;
  else if (value > 1)
    value = 1.;
  
  p->value = value;
  avr_raise_irq (p->irq + IRQ_POTENTIOMETER_TEMP_VALUE_OUT, value * 1024);
}
