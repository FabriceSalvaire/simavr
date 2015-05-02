/*
 * sim_irq.h
 *
 * Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>
 *
 * This file is part of simavr.
 *
 * simavr is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * simavr is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with simavr.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * @defgroup sim_irq Internal IRQ System (simulator callback)
 * @{
 * 
 * @note IRQ stands usually for Interrupt Request, but here it has nothing to do with AVR
 * interrupts.  It is in fact a messaging system bewteen the simulator and the user code, similar to
 * the Qt signal-slot architecture. "Raise" means "emit" in Qt language.
 *
 * This subsystem allows any piece of code to "register" a hook to be called when an IRQ is raised.
 * The IRQ definition is up to the module defining it, for example a IOPORT pin change might be an
 * IRQ in which case any piece of code can be notified when a pin has changed state
 * 
 * The notify hooks are chained, and duplicates are filtered out so you can't register a
 * notify hook twice on one particular IRQ
 * 
 * IRQ calling order is not defined, so don't rely on it.
 * 
 * IRQ hook needs to be registered in reset() handlers, i.e. after all modules init() bits
 * have been called, to prevent race condition of the initialization order.
 *
 */

#ifndef __SIM_IRQ_H__
#define __SIM_IRQ_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  struct avr_irq_t;

  /// Callback Prototype
  /// @param irq is the caller
  /// @param value is the raised value
  /// @param param is the user data provided at the IRQ registration
  typedef void (*avr_irq_notify_t) (struct avr_irq_t * irq, uint32_t value, void *param);

  enum
    {
      IRQ_FLAG_NOT = (1 << 0),   ///< change polarity of the IRQ
      IRQ_FLAG_FILTERED = (1 << 1),   ///< do not "notify" if "value" is the same as previous raise
      IRQ_FLAG_ALLOC = (1 << 2),   ///< this irq structure was malloced via avr_alloc_irq
      IRQ_FLAG_INIT = (1 << 3),   ///< this irq hasn't been used yet
    };

  /// IRQ Pool structure
  typedef struct avr_irq_pool_t
  {
    int count;   ///< number of irqs living in the pool
    struct avr_irq_t **irq;   ///< irqs belonging in this pool
  } avr_irq_pool_t;

  /// Public IRQ structure
  typedef struct avr_irq_t
  {
    struct avr_irq_pool_t *pool;   // TODO: migration in progress
    const char *name;
    uint32_t irq;   ///< any value the user needs
    uint32_t value;   ///< current value
    uint8_t flags;   ///< IRQ_* flags
    struct avr_irq_hook_t *hook;   ///< list of hooks to be notified
  } avr_irq_t;

  /// Allocates 'count' IRQs, initializes their "irq" starting from 'base' and increment
  avr_irq_t *avr_alloc_irq (avr_irq_pool_t * pool, uint32_t base, uint32_t count,
                            const char **names /* optional */ );
  /// Free allocated IRQs
  void avr_free_irq (avr_irq_t * irq, uint32_t count);

  /// Init 'count' IRQs, initializes their "irq" starting from 'base' and increment
  void avr_init_irq (avr_irq_pool_t * pool, avr_irq_t * irq, uint32_t base, uint32_t count,
                     const char **names /* optional */ );

  /// Raise an IRQ. I.e. call their 'hooks', and raise any chained IRQs, and set the new 'value'
  void avr_raise_irq (avr_irq_t * irq, uint32_t value);

  /// Connect a source IRQ to a destination IRQ
  void avr_connect_irq (avr_irq_t * src, avr_irq_t * dst);
  /// Disconnect a source IRQ to a destination IRQ
  void avr_unconnect_irq (avr_irq_t * src, avr_irq_t * dst);

  /// Register a notification hook (callback) for an irq
  /// @param param is anything that your want passed back as argument
  void avr_irq_register_notify (avr_irq_t * irq, avr_irq_notify_t notify, void *param);
  /// Unregister a callback for an irq
  void avr_irq_unregister_notify (avr_irq_t * irq, avr_irq_notify_t notify, void *param);

#ifdef __cplusplus
};
#endif

#endif /* __SIM_IRQ_H__ */
/// @} end of sim_irq group
