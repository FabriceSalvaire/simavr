/*
 *  avr_timer.c
 * 
 *  Handles the 8 bits and 16 bits AVR timer.
 *  Handles
 *  + CDC
 *  + Fast PWM
 * 
 *  Copyright 2008-2012 Michel Pollet <buserror@gmail.com>
 * 
 *  This file is part of simavr.
 * 
 *  simavr is free software: you can redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 * 
 *  simavr is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 *  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *  Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License along with simavr.  If not, see
 *  <http://www.gnu.org/licenses/>.
 */

/*
 * tov: time overflow
 *
 */

/**************************************************************************************************/

#include <stdio.h>

#include "avr_ioport.h"
#include "avr_timer.h"
#include "sim_time.h"

/**************************************************************************************************/

/** Get Ouput Compare Register (OCR)
 * 
 */
static uint16_t
_timer_get_ocr (avr_timer_t * timer, int comparator_index)
{
  avr_t *avr = timer->io.avr;
  avr_timer_comp_t *comparator = &(timer->comp[comparator_index]);

  // AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, timer->name);
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);

  uint8_t *sram = avr->data;
  avr_io_addr_t ocrh = comparator->r_ocrh;
  // The timers are /always/ 16 bits here, if the higher byte register is specified it's just added.
  return sram[comparator->r_ocr] | (ocrh ? (ocrh << 8) : 0);
}

/** cf. infra, duplicate
 * 
 */
static uint16_t
_timer_get_comp_ocr (struct avr_t *avr, avr_timer_comp_p comparator)
{
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);

  uint8_t *sram = avr->data;
  avr_io_addr_t ocrh = comparator->r_ocrh;
  return sram[comparator->r_ocr] | (ocrh ? (ocrh << 8) : 0);
}

/** Get Timer Counter Register (TCNT)
 * 
 */
static uint16_t
_timer_get_tcnt (avr_timer_t * timer)
{
  avr_t *avr = timer->io.avr;

  // AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, timer->name);
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);
  
  uint8_t *sram = timer->io.avr->data;
  avr_io_addr_t r_tcnth = timer->r_tcnth;
  return sram[timer->r_tcnt] | (r_tcnth ? (sram[r_tcnth] << 8) : 0);
}

/** Get Input Capture Register (ICR)
 * 
 */
static uint16_t
_timer_get_icr (avr_timer_t * timer)
{
  avr_t *avr = timer->io.avr;

  // AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, timer->name);
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);
  
  uint8_t *sram = timer->io.avr->data;
  avr_io_addr_t r_icrh = timer->r_icrh;
  return sram[timer->r_icr] | (r_icrh ? (sram[r_icrh] << 8) : 0);
}

/**
 *
 */
static avr_cycle_count_t
avr_timer_comp (avr_timer_t * p, avr_cycle_count_t when, uint8_t comparator_index)
{
  avr_t *avr = p->io.avr;

  // AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, p->name);
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);
  
  avr_timer_comp_t *comparator = &(p->comp[comparator_index]);

  avr_raise_interrupt (avr, &comparator->interrupt);

  // check output compare mode and set/clear pins
  uint8_t mode = avr_regbit_get (avr, comparator->com);
  avr_irq_t *irq = &p->io.irq[TIMER_IRQ_OUT_COMP + comparator_index];

  switch (mode)
    {
    case avr_timer_com_normal:   // Normal mode OCnA disconnected
      break;
    case avr_timer_com_toggle:   // Toggle OCnA on compare match
      if (comparator->com_pin.reg)   // we got a physical pin
        avr_raise_irq (irq, AVR_IOPORT_OUTPUT | (avr_regbit_get (avr, comparator->com_pin) ? 0 : 1));
      else   // no pin, toggle the IRQ anyway
        avr_raise_irq (irq, irq->value ? 0 : 1);
      break;
    case avr_timer_com_clear:
      avr_raise_irq (irq, 0);
      break;
    case avr_timer_com_set:
      avr_raise_irq (irq, 1);
      break;
    }

  return p->tov_cycles ?
    0 : comparator->comp_cycles ?
    when + comparator->comp_cycles : 0;
}

/**
 * 
 */
/// Check output compare mode and set/clear pins
static void
avr_timer_comp_on_tov (avr_timer_t * p, avr_cycle_count_t when, uint8_t comparator_index)
{
  avr_t *avr = p->io.avr;
  // AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, p->name);
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);
  
  avr_timer_comp_t *comparator = &(p->comp[comparator_index]);
  uint8_t mode = avr_regbit_get (avr, comparator->com);
  avr_irq_t *irq = &p->io.irq[TIMER_IRQ_OUT_COMP + comparator_index];

  switch (mode)
    {
    case avr_timer_com_normal:   // Normal mode
    case avr_timer_com_toggle:   // toggle on compare match => on tov do nothing
      break;
    case avr_timer_com_clear:   // clear on compare match => set on tov
      avr_raise_irq (irq, 1);
      break;
    case avr_timer_com_set:   //set on compare match => clear on tov
      avr_raise_irq (irq, 0);
      break;
    }
}

/**
 * 
 */
static avr_cycle_count_t
avr_timer_compa (struct avr_t *avr, avr_cycle_count_t when, void *param)
{
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);
  
  return avr_timer_comp ((avr_timer_t *) param, when, AVR_TIMER_COMPA); // similar code, cf. supra
}

/**
 * 
 */
static avr_cycle_count_t
avr_timer_compb (struct avr_t *avr, avr_cycle_count_t when, void *param)
{
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);
  
  return avr_timer_comp ((avr_timer_t *) param, when, AVR_TIMER_COMPB);
}

/**
 * 
 */
static avr_cycle_count_t
avr_timer_compc (struct avr_t *avr, avr_cycle_count_t when, void *param)
{
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);
  
  return avr_timer_comp ((avr_timer_t *) param, when, AVR_TIMER_COMPC);
}

/**
 * 
 */
// timer overflow
static avr_cycle_count_t
avr_timer_tov (struct avr_t *avr, avr_cycle_count_t when, void *param)
{
  avr_timer_t *p = (avr_timer_t *) param;
  // AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, p->name);
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);  

  int start = p->tov_base == 0;

  if (!start)
    avr_raise_interrupt (avr, &p->overflow);
  p->tov_base = when;

  static const avr_cycle_timer_t dispatch[AVR_TIMER_COMP_COUNT] =
    { avr_timer_compa, avr_timer_compb, avr_timer_compc };

  for (int compi = 0; compi < AVR_TIMER_COMP_COUNT; compi++)
    {
      if (p->comp[compi].comp_cycles)
        {
          if (p->comp[compi].comp_cycles < p->tov_cycles)
            {
              avr_timer_comp_on_tov (p, when, compi);
              avr_cycle_timer_register (avr, p->comp[compi].comp_cycles, dispatch[compi], p);
            }
          else if (p->tov_cycles == p->comp[compi].comp_cycles && !start)
            dispatch[compi] (avr, when, param);
        }
    }

  return when + p->tov_cycles;
}

/** Compute the current value of TCNT
 *
 */
static uint16_t
_avr_timer_get_current_tcnt (avr_timer_t * p)
{
  avr_t *avr = p->io.avr;

  // AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, p->name);
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);
  
  if (p->tov_cycles)
    {
      uint64_t delta_cycles = avr->cycle - p->tov_base; // uintx_t ???
      return (delta_cycles * (((uint64_t) p->tov_top) + 1)) / p->tov_cycles;
    }
  else
    return 0;
}

/** Read TCNT
 *
 */
static uint8_t
avr_timer_tcnt_read (struct avr_t *avr, avr_io_addr_t addr, void *param)
{
  avr_timer_t *p = (avr_timer_t *) param;
  
  // AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, p->name);
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);
 
  uint16_t tcnt = _avr_timer_get_current_tcnt (p);
  uint8_t *sram = avr->data;
  sram[p->r_tcnt] = tcnt; // addr == r_tcnt ???
  if (p->r_tcnth)
    sram[p->r_tcnth] = tcnt >> 8;

  // made to trigger potential watchpoints
  return avr_core_watch_read (avr, addr);
}

/**
 * 
 */
static void
avr_timer_cancel_all_cycle_timers (struct avr_t *avr, avr_timer_t * timer)
{
  // AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, timer->name);
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);
  
  avr_cycle_timer_cancel (avr, avr_timer_tov, timer);
  avr_cycle_timer_cancel (avr, avr_timer_compa, timer);
  avr_cycle_timer_cancel (avr, avr_timer_compb, timer);
  avr_cycle_timer_cancel (avr, avr_timer_compc, timer);
}

/** Write TCNT
 *
 */
static void
avr_timer_tcnt_write (struct avr_t *avr, avr_io_addr_t addr, uint8_t v, void *param)
{
  avr_timer_t *p = (avr_timer_t *) param;

  AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, p->name);
  
  avr_core_watch_write (avr, addr, v);
  uint16_t tcnt = _timer_get_tcnt (p);

  if (!p->tov_top)
    return;

  if (tcnt >= p->tov_top)
    tcnt = 0;

  // This involves some magicking
  // cancel the current timers, recalculate the "base" we should be at, reset the timer base as it
  // should, and re-schedule the timers using that base.

  avr_timer_cancel_all_cycle_timers (avr, p);

  uint64_t cycles = (tcnt * p->tov_cycles) / p->tov_top;

  // printf("%s-%c %d/%d -- cycles %d/%d\n",
  //        __FUNCTION__, p->name, tcnt, p->tov_top, (uint32_t)cycles, (uint32_t)p->tov_cycles);

  // this reset the timers bases to the new base
  if (p->tov_cycles > 1)
    {
      avr_cycle_timer_register (avr, p->tov_cycles - cycles, avr_timer_tov, p);
      p->tov_base = 0;
      avr_timer_tov (avr, avr->cycle - cycles, p);
    }

  // tcnt = ((avr->cycle - p->tov_base) * p->tov_top) / p->tov_cycles;
  // printf("%s-%c new tnt derive to %d\n", __FUNCTION__, p->name, tcnt);    
}

/** Configure a timer
 *
 */
static void
_avr_timer_configure (avr_timer_t * p, uint32_t clock, uint32_t top)
{
  avr_t *avr = p->io.avr;
  
  // AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, p->name);
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);
  
  float overflow_frequency = clock / (float) (top + 1);

  p->tov_top = top;
  p->tov_cycles = avr_hz_to_cycles(avr, overflow_frequency);

  AVR_LOG (avr, LOG_TRACE,
	   "TIMER: %s-%c TOP %.2f Hz = %d cycles = %d usec\n",
	   __FUNCTION__, p->name,
	   overflow_frequency, (int) p->tov_cycles, (int) avr_cycles_to_usec (avr, p->tov_cycles));

  for (int comparator_index = 0; comparator_index < AVR_TIMER_COMP_COUNT; comparator_index++)
    {
      avr_timer_comp_t *comparator = &(p->comp[comparator_index]);

      if (!comparator->r_ocr)
        continue;

      uint32_t ocr = _timer_get_ocr (p, comparator_index);
      float fc = clock / (float) (ocr + 1);

      comparator->comp_cycles = 0;
      // printf("%s-%c clock %d top %d OCR%c %d\n", __FUNCTION__, p->name, clock, top, 'A'+comparator_index, ocr);

      if (ocr && ocr <= top)
        {
          comparator->comp_cycles = avr_hz_to_cycles(avr, fc);
	  AVR_LOG (avr, LOG_TRACE,
		   "TIMER: %s-%c %c %.2f Hz = %d cycles\n",
		   __FUNCTION__, p->name,
		   'A' + comparator_index, fc, (int) comparator->comp_cycles);
        }
    }

  if (p->tov_cycles > 1)
    {
      avr_cycle_timer_register (avr, p->tov_cycles, avr_timer_tov, p);
      // calling it once, with when == 0 tells it to arm the A/B/C timers if needed
      p->tov_base = 0;
      avr_timer_tov (avr, avr->cycle, p);
    }
}

/**
 * 
 */
static void
avr_timer_reconfigure (avr_timer_t * p)
{
  avr_t *avr = p->io.avr;
  // AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, p->name);
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);

  // cancel everything
  p->comp[AVR_TIMER_COMPA].comp_cycles = 0;
  p->comp[AVR_TIMER_COMPB].comp_cycles = 0;
  p->comp[AVR_TIMER_COMPC].comp_cycles = 0;
  p->tov_cycles = 0;

  avr_timer_cancel_all_cycle_timers (avr, p);

  switch (p->wgm_op_mode_kind)
    {
    case avr_timer_wgm_normal:
    case avr_timer_wgm_fc_pwm:
    case avr_timer_wgm_fast_pwm:
      _avr_timer_configure (p, p->cs_div_clock, p->wgm_op_mode_size);
      break;
    case avr_timer_wgm_ctc:
      _avr_timer_configure (p, p->cs_div_clock, _timer_get_ocr (p, AVR_TIMER_COMPA));
      break;
    case avr_timer_wgm_pwm:
      {
        uint16_t top = (p->mode.top == avr_timer_wgm_reg_ocra) ?
          _timer_get_ocr (p, AVR_TIMER_COMPA) : _timer_get_icr (p);
        _avr_timer_configure (p, p->cs_div_clock, top);
      }
      break;
    default:
      {
        uint8_t mode = avr_regbit_get_array (avr, p->wgm, ARRAY_SIZE (p->wgm));
	AVR_LOG (avr, LOG_WARNING, "TIMER: %s-%c unsupported timer mode wgm=%d (%d)\n",
                 __FUNCTION__, p->name, mode, p->mode.kind);
      }
    }
}

/**
 * 
 */
static void
avr_timer_write_ocr (struct avr_t *avr, avr_io_addr_t addr, uint8_t v, void *param)
{
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);

  avr_timer_comp_p comp = (avr_timer_comp_p) param;
  avr_timer_t *timer = comp->timer;
  uint16_t oldv;

  /* check to see if the OCR values actually changed */
  oldv = _timer_get_comp_ocr (avr, comp);
  avr_core_watch_write (avr, addr, v);

  switch (timer->wgm_op_mode_kind)
    {
    case avr_timer_wgm_normal:
      avr_timer_reconfigure (timer);
      break;
    case avr_timer_wgm_fc_pwm:   // OCR is not used here
      avr_timer_reconfigure (timer);
      break;
    case avr_timer_wgm_ctc:
      avr_timer_reconfigure (timer);
      break;
    case avr_timer_wgm_pwm:
      if (timer->mode.top != avr_timer_wgm_reg_ocra)
        avr_raise_irq (timer->io.irq + TIMER_IRQ_OUT_PWM0, _timer_get_ocr (timer, AVR_TIMER_COMPA));
      else
        avr_timer_reconfigure (timer);   // if OCRA is the top, reconfigure needed
      avr_raise_irq (timer->io.irq + TIMER_IRQ_OUT_PWM1, _timer_get_ocr (timer, AVR_TIMER_COMPB));
      break;
    case avr_timer_wgm_fast_pwm:
      if (oldv != _timer_get_comp_ocr (avr, comp))
        avr_timer_reconfigure (timer);
      avr_raise_irq (timer->io.irq + TIMER_IRQ_OUT_PWM0, _timer_get_ocr (timer, AVR_TIMER_COMPA));
      avr_raise_irq (timer->io.irq + TIMER_IRQ_OUT_PWM1, _timer_get_ocr (timer, AVR_TIMER_COMPB));
      break;
    default:
      AVR_LOG (avr, LOG_WARNING, "TIMER: %s-%c mode %d UNSUPPORTED\n", __FUNCTION__, timer->name,
               timer->mode.kind);
      avr_timer_reconfigure (timer);
      break;
    }
}

/**
 * 
 */
static void
avr_timer_write (struct avr_t *avr, avr_io_addr_t addr, uint8_t v, void *param)
{
  avr_timer_t *p = (avr_timer_t *) param;
  // AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, p->name);
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);

  uint8_t as2 = avr_regbit_get (avr, p->as2);
  uint8_t cs = avr_regbit_get_array (avr, p->cs, ARRAY_SIZE (p->cs));
  uint8_t mode = avr_regbit_get_array (avr, p->wgm, ARRAY_SIZE (p->wgm));

  avr_core_watch_write (avr, addr, v);

  uint8_t new_as2 = avr_regbit_get (avr, p->as2);
  uint8_t new_cs = avr_regbit_get_array (avr, p->cs, ARRAY_SIZE (p->cs));
  uint8_t new_mode = avr_regbit_get_array (avr, p->wgm, ARRAY_SIZE (p->wgm));

  // only reconfigure the timer if "relevant" bits have changed
  // this prevent the timer reset when changing the edge detector or other minor bits
  if (new_cs != cs || new_mode != mode || new_as2 != as2)
    {
      /* as2 */
      long clock;

      // only can exists on "asynchronous" 8 bits timers
      if (new_as2)
        clock = 32768;
      else
        clock = avr->frequency;

      /* cs */
      if (new_cs == 0)
        {
	  // cancel everything
          p->comp[AVR_TIMER_COMPA].comp_cycles = 0;
          p->comp[AVR_TIMER_COMPB].comp_cycles = 0;
          p->comp[AVR_TIMER_COMPC].comp_cycles = 0;
          p->tov_cycles = 0;

          avr_cycle_timer_cancel (avr, avr_timer_tov, p);
          avr_cycle_timer_cancel (avr, avr_timer_compa, p);
          avr_cycle_timer_cancel (avr, avr_timer_compb, p);
          avr_cycle_timer_cancel (avr, avr_timer_compc, p);

	  AVR_LOG (avr, LOG_TRACE, "TIMER: %s-%c clock turned off\n", __FUNCTION__, p->name);
          return;
        }
      p->cs_div_clock = clock >> p->cs_div[new_cs];

      /* mode */
      p->mode = p->wgm_op[new_mode];
      p->wgm_op_mode_kind = p->mode.kind;
      p->wgm_op_mode_size = (1 << p->mode.size) - 1;

      avr_timer_reconfigure (p);
    }
}

/*
 * write to the TIFR register. Watch for code that writes "1" to clear pending interrupts.
 */
static void
avr_timer_write_pending (struct avr_t *avr, avr_io_addr_t addr, uint8_t v, void *param)
{
  avr_timer_t *p = (avr_timer_t *) param;
  // AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, p->name);
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);

  // save old bits values
  uint8_t ov = avr_regbit_get (avr, p->overflow.raised);
  uint8_t ic = avr_regbit_get (avr, p->icr.raised);
  uint8_t cp[AVR_TIMER_COMP_COUNT];

  for (int compi = 0; compi < AVR_TIMER_COMP_COUNT; compi++)
    cp[compi] = avr_regbit_get (avr, p->comp[compi].interrupt.raised);

  // write the value
  avr_core_watch_write (avr, addr, v);

  // clear any interrupts & flags
  avr_clear_interrupt_if (avr, &p->overflow, ov);
  avr_clear_interrupt_if (avr, &p->icr, ic);

  for (int compi = 0; compi < AVR_TIMER_COMP_COUNT; compi++)
    avr_clear_interrupt_if (avr, &p->comp[compi].interrupt, cp[compi]);
}

/**
 * 
 */
static void
avr_timer_irq_icp (struct avr_irq_t *irq, uint32_t value, void *param)
{
  avr_timer_t *p = (avr_timer_t *) param;
  avr_t *avr = p->io.avr;
  // AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, p->name);
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);

  // input capture disabled when ICR is used as top
  if (p->mode.top == avr_timer_wgm_reg_icr)
    return;
  int bing = 0;
  if (avr_regbit_get (avr, p->ices))
    {   // rising edge
      if (!irq->value && value)
        bing++;
    }
  else
    {   // default, falling edge
      if (irq->value && !value)
        bing++;
    }
  if (!bing)
    return;
  // get current TCNT, copy it to ICR, and raise interrupt
  uint16_t tcnt = _avr_timer_get_current_tcnt (p);
  avr->data[p->r_icr] = tcnt;
  if (p->r_icrh)
    avr->data[p->r_icrh] = tcnt >> 8;
  avr_raise_interrupt (avr, &p->icr);
}

/**
 * 
 */
static void
avr_timer_reset (avr_io_t * port)
{
  avr_timer_t *p = (avr_timer_t *) port;
  avr_t *avr = p->io.avr;
  // AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, p->name);
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);

  avr_timer_cancel_all_cycle_timers (p->io.avr, p);

  // check to see if the comparators have a pin output. If they do, (try) to get the ioport
  // corresponding IRQ and connect them they will automagically be triggered when the comparator
  // raises it's own IRQ
  for (int compi = 0; compi < AVR_TIMER_COMP_COUNT; compi++)
    {
      p->comp[compi].comp_cycles = 0;

      avr_ioport_getirq_t req = {
        .bit = p->comp[compi].com_pin
      };
      if (avr_ioctl (port->avr, AVR_IOCTL_IOPORT_GETIRQ_REGBIT, &req) > 0)
        {
	  // cool, got an IRQ
	  // printf("%s-%c COMP%c Connecting PIN IRQ %d\n", __FUNCTION__, p->name, 'A'+compi, req.irq[0]->irq);
          avr_connect_irq (&port->irq[TIMER_IRQ_OUT_COMP + compi], req.irq[0]);
        }
    }
  avr_ioport_getirq_t req = {
    .bit = p->icp
  };
  if (avr_ioctl (port->avr, AVR_IOCTL_IOPORT_GETIRQ_REGBIT, &req) > 0)
    {
      // cool, got an IRQ for the input capture pin
      // printf("%s-%c ICP Connecting PIN IRQ %d\n", __FUNCTION__, p->name, req.irq[0]->irq);
      avr_irq_register_notify (req.irq[0], avr_timer_irq_icp, p);
    }
}

/**************************************************************************************************/

static const char *irq_names[TIMER_IRQ_COUNT] = {
  [TIMER_IRQ_OUT_PWM0] = "8>pwm0",
  [TIMER_IRQ_OUT_PWM1] = "8>pwm1",
  [TIMER_IRQ_OUT_COMP + 0] = ">compa",
  [TIMER_IRQ_OUT_COMP + 1] = ">compb",
  [TIMER_IRQ_OUT_COMP + 2] = ">compc",
};

static avr_io_t _io = {
  .kind = "timer",
  .reset = avr_timer_reset,
  .irq_names = irq_names,
};

/**
 * 
 */
void
avr_timer_init (avr_t * avr, avr_timer_t * p)
{
  // AVR_LOG (avr, LOG_TRACE, "TIMER: %s %s\n", __FUNCTION__, p->name);
  AVR_LOG (avr, LOG_TRACE, "TIMER: %s\n", __FUNCTION__);

  p->io = _io;

  avr_register_io (avr, &p->io);
  avr_register_vector (avr, &p->overflow);
  avr_register_vector (avr, &p->icr);

  // allocate this module's IRQ
  avr_io_setirqs (&p->io, AVR_IOCTL_TIMER_GETIRQ (p->name), TIMER_IRQ_COUNT, NULL);

  // marking IRQs as "filtered" means they don't propagate if the new value raised is the same as
  // the last one.. in the case of the pwm value it makes sense not to bother.
  p->io.irq[TIMER_IRQ_OUT_PWM0].flags |= IRQ_FLAG_FILTERED;
  p->io.irq[TIMER_IRQ_OUT_PWM1].flags |= IRQ_FLAG_FILTERED;

  avr_regbit_t *wgm = p->wgm;
  if (wgm[0].reg)   // these are not present on older AVRs
    avr_register_io_write (avr, wgm[0].reg, avr_timer_write, p);
  if (wgm[1].reg
      && (wgm[1].reg != wgm[0].reg))
    avr_register_io_write (avr, wgm[1].reg, avr_timer_write, p);
  if (wgm[2].reg
      && (wgm[2].reg != wgm[0].reg) && (wgm[2].reg != wgm[1].reg))
    avr_register_io_write (avr, wgm[2].reg, avr_timer_write, p);
  if (wgm[3].reg
      && (wgm[3].reg != wgm[0].reg) && (wgm[3].reg != wgm[1].reg) && (wgm[3].reg != wgm[2].reg))
    avr_register_io_write (avr, wgm[3].reg, avr_timer_write, p);

  avr_regbit_t *cs = p->cs;
  avr_register_io_write (avr, cs[0].reg, avr_timer_write, p);
  if (cs[1].reg
      && (cs[1].reg != cs[0].reg))
    avr_register_io_write (avr, cs[1].reg, avr_timer_write, p);
  if (cs[2].reg
      && (cs[2].reg != cs[0].reg) && (cs[2].reg != cs[1].reg))
    avr_register_io_write (avr, cs[2].reg, avr_timer_write, p);
  if (cs[3].reg
      && (cs[3].reg != cs[0].reg) && (cs[3].reg != cs[1].reg) && (cs[3].reg != cs[2].reg))
    avr_register_io_write (avr, cs[3].reg, avr_timer_write, p);

  if (p->as2.reg)   // as2 signifies timer/counter 2... therefore must check for register.
    avr_register_io_write (avr, p->as2.reg, avr_timer_write, p);

  // this assumes all the "pending" interrupt bits are in the same register.
  // Might not be true on all devices ?
  avr_register_io_write (avr, p->overflow.raised.reg, avr_timer_write_pending, p);
  
  // Even if the timer is 16 bits, we don't care to have watches on the high bytes because the
  // datasheet says that the low address is always the trigger.
  for (int compi = 0; compi < AVR_TIMER_COMP_COUNT; compi++)
    {
      p->comp[compi].timer = p;

      avr_register_vector (avr, &p->comp[compi].interrupt);

      if (p->comp[compi].r_ocr)   // not all timers have all comparators
        avr_register_io_write (avr, p->comp[compi].r_ocr, avr_timer_write_ocr, &p->comp[compi]);
    }
  avr_register_io_write (avr, p->r_tcnt, avr_timer_tcnt_write, p);
  avr_register_io_read (avr, p->r_tcnt, avr_timer_tcnt_read, p);
}

/**************************************************************************************************/
