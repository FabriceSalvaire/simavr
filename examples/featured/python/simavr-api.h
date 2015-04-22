/**************************************************************************************************/

enum
  {
    cpu_Limbo = 0,   // before initialization is finished
    cpu_Stopped,   // all is stopped, timers included
    cpu_Running,   // we're free running
    cpu_Sleeping,   // we're now sleeping until an interrupt
    cpu_Step,   // run ONE instruction, then...
    cpu_StepDone,   // tell gdb it's all OK, and give it registers
    cpu_Done,   // avr software stopped gracefully
    cpu_Crashed,   // avr software crashed (watchdog fired)
  };

enum
  {
    LOG_OUTPUT = 0,
    LOG_ERROR,
    LOG_WARNING,
    LOG_TRACE,
  };

typedef struct avr_irq_pool_t {...;} avr_irq_pool_t;
typedef struct avr_irq_t {...;} avr_irq_t;

typedef struct avr_t {
  // uint8_t trace:1, log:2;
  ...;
} avr_t;

typedef struct elf_firmware_t {
  char mmcu[64];
  uint32_t frequency;
  uint32_t vcc, avcc, aref;
  ...;
} elf_firmware_t;

typedef uint64_t avr_cycle_count_t;
typedef uint16_t avr_io_addr_t;

typedef avr_cycle_count_t (*avr_cycle_timer_t) (struct avr_t * avr, avr_cycle_count_t when, void *param);
typedef void (*avr_irq_notify_t) (struct avr_irq_t * irq, uint32_t value, void *param);
typedef void (*avr_io_write_t) (struct avr_t * avr, avr_io_addr_t addr, uint8_t v, void *param);

/**************************************************************************************************
 *
 *  sim_avr.h
 *
 */

// initializes a new AVR instance. Will call the IO registers init(), and then reset()
int avr_init (avr_t * avr);

// locate the maker for mcu "name" and allocates a new avr instance
avr_t *avr_make_mcu_by_name (const char *name);

// run one cycle of the AVR, sleep if necessary
int avr_run (avr_t * avr);

/**************************************************************************************************
 *
 * sim_cycle_timers.h
 *
 */

// cancel a previously set timer
void avr_cycle_timer_cancel (struct avr_t *avr, avr_cycle_timer_t timer, void *param);

// register for calling 'timer' in 'when' cycles
void avr_cycle_timer_register (struct avr_t *avr, avr_cycle_count_t when, avr_cycle_timer_t timer, void *param);

// register a timer to call in 'when' usec
void avr_cycle_timer_register_usec (struct avr_t *avr, uint32_t when, avr_cycle_timer_t timer, void *param);

/**************************************************************************************************
 *
 * sim_elf.h
 *
 */

int elf_read_firmware (const char *file, elf_firmware_t * firmware);
void avr_load_firmware (avr_t * avr, elf_firmware_t * firmware);

/**************************************************************************************************
 *
 * sim_irq.h
 *
 */

// allocates 'count' IRQs, initializes their "irq" starting from 'base' and increment
avr_irq_t *avr_alloc_irq (avr_irq_pool_t * pool, uint32_t base, uint32_t count, const char **names /* optional */ );

// this connects a "source" IRQ to a "destination" IRQ
void avr_connect_irq (avr_irq_t * src, avr_irq_t * dst);

// 'raise' an IRQ. Ie call their 'hooks', and raise any chained IRQs, and set the new 'value'
void avr_raise_irq (avr_irq_t * irq, uint32_t value);

// register a notification 'hook' for 'irq' -- 'param' is anything that your want passed back as argument
void avr_irq_register_notify (avr_irq_t * irq, avr_irq_notify_t notify, void *param);

/**************************************************************************************************
 *
 * sim_io.h
 *
 */

// call every IO modules until one responds to this
int avr_ioctl (avr_t * avr, uint32_t ctl, void *io_param);

// get the specific irq for a module, check AVR_IOCTL_IOPORT_GETIRQ for example
struct avr_irq_t *avr_io_getirq (avr_t * avr, uint32_t ctl, int index);

// register a callback for when the IO register is written. callback has to set the memory itself
void avr_register_io_write (avr_t * avr, avr_io_addr_t addr, avr_io_write_t write, void *param);

/**************************************************************************************************
 *
 * sim_time.h
 * 
 */

// converts a number of usec to a number of machine cycles, at current speed
static inline avr_cycle_count_t avr_usec_to_cycles (struct avr_t *avr, uint32_t usec);

/**************************************************************************************************/

uint32_t avr_ioctl_ioport_getirq(const char *port);
void set_log_level(avr_t *avr, uint8_t level);

/***************************************************************************************************
 * 
 * End
 * 
 **************************************************************************************************/
