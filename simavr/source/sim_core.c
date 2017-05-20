/*
 * sim_core.c
 *
 * Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>
 *
 * This file is part of simavr.
 *
 * simavr is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * simavr is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with simavr.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**************************************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**************************************************************************************************/

#include "logging.h"

#include "avr_flash.h"
#include "avr_watchdog.h"
#include "sim_avr.h"
#include "sim_core.h"
#include "sim_gdb.h"

/**************************************************************************************************/

/// SREG bit names
const char *_sreg_bit_name = "cznvshti";

/**************************************************************************************************/

/*
 * Handle "touching" registers, marking them changed.
 * This is used only for debugging purposes to be able to print the effects of each instructions on
 * registers
 */
#if CONFIG_SIMAVR_TRACE

#define T(w) w

/// Mark register as touched
// The touched structure map each register to a bit in a 32-bit array:
//    r -> offset: r / 32, bit: r % 32
#define REG_TOUCH(a, r) \
  (a)->trace_data->touched[(r) >> 5] |= (1 << ((r) & 0x1f))
/// Test if register is touched
#define REG_ISTOUCHED(a, r) \
  ((a)->trace_data->touched[(r) >> 5] & (1 << ((r) & 0x1f)))

/// This allows a "special case" to skip instruction tracing when in these symbols since printf() is
/// useful to have, but generates a lot of cycles.
int
dont_trace (const char *name)
{
  return (!strcmp (name, "uart_putchar") ||
          !strcmp (name, "fputc") ||
          !strcmp (name, "printf") ||
          !strcmp (name, "vfprintf") ||
          !strcmp (name, "__ultoa_invert") ||
          !strcmp (name, "__prologue_saves__") ||
	  !strcmp (name, "__epilogue_restores__"));
}

int donttrace = 0;

/// Print current instruction
#define STATE(_f, args...) {                                            \
    if (avr->trace) {                                                   \
      struct avr_trace_data_t *trace_data = avr->trace_data;		\
      if (trace_data->codeline &&					\
	  trace_data->codeline[avr->pc >> 1]) {				\
	const char * symbol = trace_data->codeline[avr->pc >> 1]->symbol; \
	int dont = 0 && dont_trace(symbol);				\
	if (dont != donttrace) {					\
	  donttrace = dont;						\
	  DUMP_REG();							\
	}								\
	if (donttrace == 0)						\
	  printf("%04x @%" PRI_avr_cycle_count ": %-25s " _f,		\
                 avr->pc, avr->cycle, symbol, ## args);			\
      } else								\
	printf("%s: %04x @%" PRI_avr_cycle_count ": " _f,		\
	       __FUNCTION__, avr->pc, avr->cycle, ## args);		\
    }                                                                   \
  }

/// Print Status Register
#define SREG()								\
  if (avr->trace && donttrace == 0) {					\
    printf("%04x: \t\t\t\t\t\t\t\t\tSREG = ", avr->pc);                 \
    for (int _sbi = 0; _sbi < 8; _sbi++)                                \
      printf("%c",							\
	     avr->sreg[_sbi] ? toupper(_sreg_bit_name[_sbi]) : '.');	\
    printf("\n");                                                       \
  }

void
crash (avr_t * avr)
{
  DUMP_REG ();
  printf ("*** CYCLE %" PRI_avr_cycle_count "PC %04x\n", avr->cycle, avr->pc);

  for (int i = OLD_PC_SIZE - 1; i > 0; i--)
    {
      int pci = (avr->trace_data->old_pci + i) & 0xf;
      printf (FONT_RED "*** %04x: %-25s RESET -%d; sp %04x\n" FONT_DEFAULT,
              avr->trace_data->old[pci].pc,
              avr->trace_data->codeline ?
	      avr->trace_data->codeline[avr->trace_data->old[pci].pc >> 1]->symbol : "unknown",
	      OLD_PC_SIZE - i, avr->trace_data->old[pci].sp);
    }

  printf ("Stack Ptr %04x/%04x = %d \n",
	  _avr_sp_get (avr), avr->ramend, avr->ramend - _avr_sp_get (avr));
  DUMP_STACK ();

  avr_sadly_crashed (avr, 0);
}
#else
#define T(w)
#define REG_TOUCH(a, r)
#define STATE(_f, args...)
#define SREG()

void
crash (avr_t * avr)
{
  avr_sadly_crashed (avr, 0);
}
#endif

/**************************************************************************************************/

/// Write a value in SRAM and handle GDB watchpoints
void
avr_core_watch_write (avr_t * avr, uint16_t addr, uint8_t value)
{
  if (addr > avr->ramend)
    {
      AVR_LOG (avr, LOG_ERROR,
               "CORE: *** Invalid write address PC=%04x SP=%04x O=%04x Address %04x=%02x out of ram\n",
               avr->pc, _avr_sp_get (avr), _avr_read_instruction(avr, avr->pc),
               addr, value);
      crash (avr);
    }
  if (addr < 32)
    {
      AVR_LOG (avr, LOG_ERROR,
               "CORE: *** Invalid write address PC=%04x SP=%04x O=%04x Address %04x=%02x low registers\n",
               avr->pc, _avr_sp_get (avr), _avr_read_instruction(avr, avr->pc),
               addr, value);
      crash (avr);
    }
#if AVR_STACK_WATCH
  // This checks that the current "function" is not doctoring the stack frame that is located
  // higher on the stack than it should be. It's a sign of code that has overrun it's stack
  // frame and is munching on it's own return address.
  if (avr->trace_data->stack_frame_index > 1
      && addr > avr->trace_data->stack_frame[avr->trace_data->stack_frame_index - 2].sp)
    {
      printf (FONT_RED "%04x : munching stack SP %04x, A=%04x <= %02x\n" FONT_DEFAULT, avr->pc,
              _avr_sp_get (avr), addr, value);
    }
#endif

  if (avr->gdb)
    avr_gdb_handle_watchpoints (avr, addr, AVR_GDB_WATCH_WRITE);

  avr->data[addr] = value;
}

/// Read a value in SRAM and handle GDB watchpoints
uint8_t
avr_core_watch_read (avr_t * avr, uint16_t addr)
{
  if (addr > avr->ramend)
    {
      AVR_LOG (avr, LOG_ERROR,
               FONT_RED
               "CORE: *** Invalid read address PC=%04x SP=%04x O=%04x Address %04x out of ram (%04x)\n"
               FONT_DEFAULT, avr->pc, _avr_sp_get (avr),
               _avr_read_instruction(avr, avr->pc), addr, avr->ramend);
      crash (avr);
    }

  if (avr->gdb)
    avr_gdb_handle_watchpoints (avr, addr, AVR_GDB_WATCH_READ);

  return avr->data[addr];
}

/// Set a register (r < 256)
/// if it's an IO register (> 31) also (try to) call any callback that was registered to track
/// changes to that register.
static inline void
_avr_set_r (avr_t * avr, uint16_t r, uint8_t value)
{
  REG_TOUCH (avr, r);

  uint8_t *sram = avr->data;
  if (r == R_SREG)
    {
      sram[R_SREG] = value;
      // unsplit the SREG
      SET_SREG_FROM (avr, value);
      SREG ();
    }
  else if (r >= IO_START_ADDRESS)   // then IO register
    {
      avr_io_addr_t io = AVR_DATA_TO_IO (r);
      avr_io_callback_data_t *io_callback_data = &(avr->io[io]);
      struct avr_irq_t *irq = io_callback_data->irq;
      avr_io_write_callback_data_t *write_callback_data = &(io_callback_data->w);
      avr_io_write_t write_callback = write_callback_data->c;
      if (write_callback)
        write_callback (avr, r, value, write_callback_data->param);
      else
        sram[r] = value;
      if (irq)
        {
          avr_raise_irq (irq + AVR_IOMEM_IRQ_ALL, value);
          for (size_t i = 0; i < 8; i++)
            avr_raise_irq (irq + i, BIT_VALUE(value, i));
        }
    }
  else   // general purpose register
    sram[r] = value;
}

/// Set any address to a value; split between registers and SRAM
static inline void
_avr_set_ram (avr_t * avr, uint16_t addr, uint8_t value)
{
  if (addr < SRAM_START_ADDRESSS)    //! right for all devices ???
    _avr_set_r (avr, addr, value);
  else
    avr_core_watch_write (avr, addr, value);
}

/// Get a value from SRAM.
static inline uint8_t
_avr_get_ram (avr_t * avr, uint16_t addr)
{
  uint8_t *sram = avr->data;
  if (addr == R_SREG)
    {
      // SREG is special it's reconstructed when read while the core itself uses the "shortcut" array
      READ_SREG_INTO (avr, sram[R_SREG]);
    }
  else if (is_io_register(addr))
    {
      avr_io_addr_t io = AVR_DATA_TO_IO (addr);
      avr_io_callback_data_t *io_callback_data = &(avr->io[io]);
      struct avr_irq_t *irq = io_callback_data->irq;
      avr_io_read_callback_data_t *read_callback_data = &(io_callback_data->r);
      avr_io_read_t read_callback = read_callback_data->c;
      if (read_callback)
        sram[addr] = read_callback (avr, addr, read_callback_data->param);
      if (irq)
        {
          uint8_t value = sram[addr];
          avr_raise_irq (irq + AVR_IOMEM_IRQ_ALL, value);   //! duplicated code
          for (int i = 0; i < 8; i++)
            avr_raise_irq (irq + i, BIT_VALUE(value, i));
        }
    }
  return avr_core_watch_read (avr, addr);
}

/// Stack pointer access
inline uint16_t
_avr_sp_get (avr_t * avr)
{
  return read_uint16_lh(avr->data, R_SPL, R_SPH);
}

inline void
_avr_sp_set (avr_t * avr, uint16_t sp)
{
  _avr_set_r (avr, R_SPL, sp);
  _avr_set_r (avr, R_SPH, sp >> 8);
}

/// Stack push accessors.
static inline void
_avr_push8 (avr_t * avr, uint16_t v)
{
  uint16_t sp = _avr_sp_get (avr);
  _avr_set_ram (avr, sp, v);
  _avr_sp_set (avr, sp - 1);
}

static inline uint8_t
_avr_pop8 (avr_t * avr)
{
  uint16_t sp = _avr_sp_get (avr) + 1;
  uint8_t res = _avr_get_ram (avr, sp);
  _avr_sp_set (avr, sp);
  return res;
}

int
_avr_push_addr (avr_t * avr, avr_flashaddr_t addr)
{
  uint16_t sp = _avr_sp_get (avr);
  addr >>= 1;
  for (int i = 0; i < avr->address_size; i++, addr >>= 8, sp--)
    _avr_set_ram (avr, sp, addr);
  _avr_sp_set (avr, sp);
  return avr->address_size;
}

avr_flashaddr_t
_avr_pop_addr (avr_t * avr)
{
  uint16_t sp = _avr_sp_get (avr) + 1;
  avr_flashaddr_t res = 0;
  for (int i = 0; i < avr->address_size; i++, sp++)
    res = (res << 8) | _avr_get_ram (avr, sp);
  res <<= 1;
  _avr_sp_set (avr, sp - 1);
  return res;
}

/// "Pretty" register names
const char *reg_names[255] = {
  [R_XH] = "XH", [R_XL] = "XL",
  [R_YH] = "YH", [R_YL] = "YL",
  [R_ZH] = "ZH", [R_ZL] = "ZL",
  [R_SPH] = "SPH", [R_SPL] = "SPL",
  [R_SREG] = "SREG",
};

const char *
avr_regname (uint8_t reg)
{
  // cache the register name for later use
  if (!reg_names[reg])
    {
      char tt[16];
      if (reg < 32)
	// then it corresponds to one of the 32 general purpose 8-bit registers of the AVR
	// 0x20 offset in the register mapping
        sprintf (tt, "r%d", reg);
      else
	// an IO register
        sprintf (tt, "io:%02x", reg);
      reg_names[reg] = strdup (tt);
    }
  return reg_names[reg];
}

/// Called when an invalid opcode is decoded
static void
_avr_invalid_opcode (avr_t * avr)
{
#if CONFIG_SIMAVR_TRACE
  printf (FONT_RED "*** %04x: %-25s Invalid Opcode SP=%04x O=%04x \n" FONT_DEFAULT,
          avr->pc, avr->trace_data->codeline[avr->pc >> 1]->symbol,
	  _avr_sp_get (avr), _avr_read_instruction(avr, avr->pc));
#else
  AVR_LOG (avr, LOG_ERROR, FONT_RED "CORE: *** %04x: Invalid Opcode SP=%04x O=%04x \n" FONT_DEFAULT,
           avr->pc, _avr_sp_get (avr), _avr_read_instruction(avr, avr->pc));
#endif
}

#if CONFIG_SIMAVR_TRACE
/// Dump changed registers when tracing
// called from avr_callback_run_raw and avr_callback_run_gdb
void
avr_dump_state (avr_t * avr)
{
  if (!avr->trace || donttrace)
    return;

  // Dump only if any first 96 (3*32) registers are modified.
  // Why ??? AVR have at least 96 ? Bigger ones ?
  int do_it = 0;
  for (size_t i = 0; i < 3 && !do_it; i++)
    if (avr->trace_data->touched[i])
      {
	do_it = 1;
	break;
      }
  if (!do_it)
    return;

  // Ensure 16-bit registers are marked touched
  const int r16[] = { R_SPL, R_XL, R_YL, R_ZL };
  for (int i = 0; i < 4; i++)
    if (REG_ISTOUCHED (avr, r16[i]) || REG_ISTOUCHED (avr, r16[i] + 1))
      {
        REG_TOUCH (avr, r16[i]);
        REG_TOUCH (avr, r16[i] + 1);
      }
  
  printf ("                                       ->> ");
  for (int i = 0; i < 3 * 32; i++)
    if (REG_ISTOUCHED (avr, i))
      printf ("%s=%02x ", avr_regname (i), avr->data[i]);
  printf ("\n");
}
#endif

/**************************************************************************************************/

/*
 * Operand patterns:
 *   _ means used for opcode
 *                            count
 *   _2 q1 _1 q2 _1 d5 _1 q3   2
 *   _2 q1 _1 q2 _1 r5 _1 q3   2 similar as before (d -> r)
 *
 *   _4 K4 d4 K4               7
 *   _4 k12                    2
 *
 *   _5 A2 d5 A4               1
 *   _5 A2 r5 A4               1 similar as before(d -> r)
 *   _5 k3 d4 k4               2
 *
 *   _6 r1 d5 r4              12
 *   _6 k7 _3                 18
 *   _6 k7 s3                  2
 *   _6 d10                    4
 *
 *   _7 d5 _1 b3               2
 *   _7 r5 _1 b3               2
 *   _7 k5 _3 k1               2
 *   _7 d5 _4                 25
 *   _7 r5 _4                 13
 *
 *   _8 K2 d2 K4               2
 *   _8 K4 _4                  1
 *   _8 d4 _4                  1 similar as before (K -> d)
 *   _8 d4 r4                  2
 *   _8 A5 b3                  4
 *
 *   _9 d3 _1 r3               4
 *   _9 s3 _4                  2
 */

#define get_d5(o) \
  const uint8_t d = (o >> 4) & 0x1f;

#define get_vd5(o)                              \
  get_d5(o)                                     \
  const uint8_t vd = avr->data[d];

#define get_r5(o) \
  const uint8_t r = ((o >> 5) & 0x10) | (o & 0xf);

#define get_d5_a6(o)                                            \
  get_d5(o);                                                    \
  const uint8_t A = ((((o >> 9) & 3) << 4) | ((o) & 0xf)) + 32;

#define get_vd5_s3(o)                           \
  get_vd5(o);                                   \
  const uint8_t s = o & 7;

#define get_vd5_s3_mask(o)                      \
  get_vd5_s3(o);                                \
  const uint8_t mask = 1 << s;

#define get_vd5_vr5(o)                                  \
  get_r5(o);                                            \
  get_d5(o);                                            \
  const uint8_t vd = avr->data[d], vr = avr->data[r];

#define get_d5_vr5(o)                           \
  get_d5(o);                                    \
  get_r5(o);                                    \
  const uint8_t vr = avr->data[r];

#define get_h4_k8(o)                                    \
  const uint8_t h = 16 + ((o >> 4) & 0xf);              \
  const uint8_t k = ((o & 0x0f00) >> 4) | (o & 0xf);

#define get_vh4_k8(o)                           \
  get_h4_k8(o)                                  \
  const uint8_t vh = avr->data[h];

#define get_d5_q6(o)                                                    \
  get_d5(o)                                                             \
  const uint8_t q = ((o & 0x2000) >> 8) | ((o & 0x0c00) >> 7) | (o & 0x7);

#define get_io5(o) \
  const uint8_t io = ((o >> 3) & 0x1f) + 32;

#define get_io5_b3(o)                           \
  get_io5(o);                                   \
  const uint8_t b = o & 0x7;

#define get_io5_b3mask(o)                       \
  get_io5(o);                                   \
  const uint8_t mask = 1 << (o & 0x7);

// const int16_t o = ((int16_t)(op << 4)) >> 3;   // CLANG BUG!
#define get_o12(op) \
  const int16_t o = ((int16_t)((op << 4) & 0xffff)) >> 3;

#define get_vp2_k6(o)                                           \
  const uint8_t p = 24 + ((o >> 3) & 0x6);                      \
  const uint8_t k = ((o & 0x00c0) >> 2) | (o & 0xf);            \
  const uint16_t vp = read_uint16(avr->data, p);

#define get_sreg_bit(o) \
  const uint8_t b = (o >> 4) & 7;

/**************************************************************************************************/

/*
 * Add a "jump" address to the jump trace buffer
 */
#if CONFIG_SIMAVR_TRACE
#define TRACE_JUMP()                                                    \
  avr->trace_data->old[avr->trace_data->old_pci].pc = avr->pc;          \
  avr->trace_data->old[avr->trace_data->old_pci].sp = _avr_sp_get(avr); \
  avr->trace_data->old_pci = (avr->trace_data->old_pci + 1) & (OLD_PC_SIZE-1); \

#if AVR_STACK_WATCH
#define STACK_FRAME_PUSH()                                              \
  avr->trace_data->stack_frame[avr->trace_data->stack_frame_index].pc = avr->pc; \
  avr->trace_data->stack_frame[avr->trace_data->stack_frame_index].sp = _avr_sp_get(avr); \
  avr->trace_data->stack_frame_index++;
#define STACK_FRAME_POP()                       \
  if (avr->trace_data->stack_frame_index > 0)   \
    avr->trace_data->stack_frame_index--;
#else
#define STACK_FRAME_PUSH()
#define STACK_FRAME_POP()
#endif
#else /* CONFIG_SIMAVR_TRACE */
#define TRACE_JUMP()
#define STACK_FRAME_PUSH()
#define STACK_FRAME_POP()
#endif

/**************************************************************************************************
 *
 * Helper functions for calculating the status register bit values.
 * See the Atmel data sheet for the instruction set for more info.
 *
 **************************************************************************************************/

static void
_avr_flags_zns (struct avr_t *avr, uint8_t res)
{
  avr->sreg[S_Z] = res == 0;
  avr->sreg[S_N] = (res >> 7) & 1;
  avr->sreg[S_S] = avr->sreg[S_N] ^ avr->sreg[S_V];
}

static void
_avr_flags_zns16 (struct avr_t *avr, uint16_t res)
{
  avr->sreg[S_Z] = res == 0;
  avr->sreg[S_N] = (res >> 15) & 1;
  avr->sreg[S_S] = avr->sreg[S_N] ^ avr->sreg[S_V];
}

static void
_avr_flags_add_zns (struct avr_t *avr, uint8_t res, uint8_t rd, uint8_t rr)
{
  // carry & half carry
  uint8_t add_carry = (rd & rr) | (rr & ~res) | (~res & rd);
  avr->sreg[S_H] = (add_carry >> 3) & 1;
  avr->sreg[S_C] = (add_carry >> 7) & 1;

  // overflow
  avr->sreg[S_V] = (((rd & rr & ~res) | (~rd & ~rr & res)) >> 7) & 1;

  // zns
  _avr_flags_zns (avr, res);
}

static void
_avr_flags_sub_zns (struct avr_t *avr, uint8_t res, uint8_t rd, uint8_t rr)
{
  // carry & half carry
  uint8_t sub_carry = (~rd & rr) | (rr & res) | (res & ~rd);
  avr->sreg[S_H] = (sub_carry >> 3) & 1;
  avr->sreg[S_C] = (sub_carry >> 7) & 1;

  // overflow
  avr->sreg[S_V] = (((rd & ~rr & ~res) | (~rd & rr & res)) >> 7) & 1;

  // zns
  _avr_flags_zns (avr, res);
}

static void
_avr_flags_Rzns (struct avr_t *avr, uint8_t res)
{
  if (res)
    avr->sreg[S_Z] = 0;
  avr->sreg[S_N] = (res >> 7) & 1;
  avr->sreg[S_S] = avr->sreg[S_N] ^ avr->sreg[S_V];
}

static void
_avr_flags_sub_Rzns (struct avr_t *avr, uint8_t res, uint8_t rd, uint8_t rr)
{
  // carry & half carry
  uint8_t sub_carry = (~rd & rr) | (rr & res) | (res & ~rd);
  avr->sreg[S_H] = (sub_carry >> 3) & 1;
  avr->sreg[S_C] = (sub_carry >> 7) & 1;

  // overflow
  avr->sreg[S_V] = (((rd & ~rr & ~res) | (~rd & rr & res)) >> 7) & 1;

  _avr_flags_Rzns (avr, res);
}

static void
_avr_flags_zcvs (struct avr_t *avr, uint8_t res, uint8_t vr)
{
  avr->sreg[S_Z] = res == 0;
  avr->sreg[S_C] = vr & 1;
  avr->sreg[S_V] = avr->sreg[S_N] ^ avr->sreg[S_C];
  avr->sreg[S_S] = avr->sreg[S_N] ^ avr->sreg[S_V];
}

static void
_avr_flags_zcnvs (struct avr_t *avr, uint8_t res, uint8_t vr)
{
  avr->sreg[S_Z] = res == 0;
  avr->sreg[S_C] = vr & 1;
  avr->sreg[S_N] = res >> 7;
  avr->sreg[S_V] = avr->sreg[S_N] ^ avr->sreg[S_C];
  avr->sreg[S_S] = avr->sreg[S_N] ^ avr->sreg[S_V];
}

static void
_avr_flags_znv0s (struct avr_t *avr, uint8_t res)
{
  avr->sreg[S_V] = 0;
  _avr_flags_zns (avr, res);
}

/**************************************************************************************************/

/*
 * 32-bit Instructions are:
 *          opcode mask
 *   CALL   0x940e 0xfe0e
 *   JMP    0x940c 0xfe0e
 *   LDS    0x9000 0xfe0f
 *   STS    0x9200 0xfe0f
 */

static inline int
_avr_is_instruction_32_bits (avr_t * avr, avr_flashaddr_t pc)
{
  uint16_t o = _avr_read_instruction(avr, pc) & 0xfc0f;
  return
    o == 0x9000 ||   // LDS Load Direct from Data Space
    o == 0x9200 ||   // STS Store Direct to Data Space
    o == 0x940c ||   // JMP Long Jump
    o == 0x940d ||   // JMP Long Jump
    o == 0x940e ||   // CALL Long Call to sub
    o == 0x940f;   // CALL Long Call to sub
}

/**************************************************************************************************/

/*
 * Main opcode decoder
 * 
 * The decoder was written by following the datasheet in no particular order.
 * As I went along, I noticed "bit patterns" that could be used to factor opcodes.
 * However, a lot of these only became apparent later on, so SOME instructions (skip of bit set etc)
 * are compact, and some could use some refactoring (the ALU ones scream to be factored).
 * I assume that the decoder could easily be 2/3 of it's current size.
 * 
 * + It lacks the "extended" XMega jumps. 
 * + It also doesn't check whether the core it's emulating is supposed to have the fancy
 *   instructions, like multiply and such.
 * 
 * The number of cycles taken by instruction has been added, but might not be entirely accurate.
 */
avr_flashaddr_t
avr_run_one (avr_t * avr)
{
 run_one_again:

#if CONFIG_SIMAVR_TRACE
  /*
   * this traces spurious reset or bad jumps
   */
  if ((avr->pc == 0 && avr->cycle > 0) ||
      avr->pc >= avr->codeend ||
      _avr_sp_get (avr) > avr->ramend)
    {
      avr->trace = 1;
      STATE ("RESET\n");
      crash (avr);
    }
  avr->trace_data->touched[0] = avr->trace_data->touched[1] = avr->trace_data->touched[2] = 0;
#endif

  // Ensure we don't crash simavr due to a bad instruction reading past the end of the flash.
  if (unlikely (avr->pc >= avr->flashend))
    {
      STATE ("CRASH\n");
      crash (avr);
      return 0;
    }

  uint8_t *sram = avr->data;
  
  uint32_t opcode = _avr_read_instruction(avr, avr->pc);
  avr_flashaddr_t new_pc = avr->pc + 2;   // future "default" pc
  int cycle = 1;

  switch (opcode & 0xf000)
    {
    case 0x0000:
      {
        switch (opcode)
          {
          case 0x0000:
            {   // NOP
              STATE ("nop\n");
            }
            break;
          default:
            {
              switch (opcode & 0xfc00)
                {
                case 0x0400:
                  {   // CPC -- Compare with carry -- 0000 01rd dddd rrrr
                    get_vd5_vr5 (opcode);
                    uint8_t res = vd - vr - avr->sreg[S_C];
                    STATE ("cpc %s[%02x], %s[%02x] = %02x\n",
			   avr_regname (d), vd, avr_regname (r), vr, res);
                    _avr_flags_sub_Rzns (avr, res, vd, vr);
                    SREG ();
                  }
                  break;
                case 0x0c00:
                  {   // ADD -- Add without carry -- 0000 11rd dddd rrrr
                    get_vd5_vr5 (opcode);
                    uint8_t res = vd + vr;
                    if (r == d)
		      {
			STATE ("lsl %s[%02x] = %02x\n", avr_regname (d), vd, res & 0xff);
		      }
                    else
		      {
			STATE ("add %s[%02x], %s[%02x] = %02x\n",
			       avr_regname (d), vd, avr_regname (r), vr, res);
		      }
                    _avr_set_r (avr, d, res);
                    _avr_flags_add_zns (avr, res, vd, vr);
                    SREG ();
                  }
                  break;
                case 0x0800:
                  {   // SBC -- Subtract with carry -- 0000 10rd dddd rrrr
                    get_vd5_vr5 (opcode);
                    uint8_t res = vd - vr - avr->sreg[S_C];
                    STATE ("sbc %s[%02x], %s[%02x] = %02x\n",
			   avr_regname (d), sram[d], avr_regname (r), sram[r], res);
                    _avr_set_r (avr, d, res);
                    _avr_flags_sub_Rzns (avr, res, vd, vr);
                    SREG ();
                  }
                  break;
                default:
                  switch (opcode & 0xff00)
                    {
                    case 0x0100:
                      {   // MOVW -- Copy Register Word -- 0000 0001 dddd rrrr
                        uint8_t d = ((opcode >> 4) & 0xf) << 1;
                        uint8_t r = ((opcode) & 0xf) << 1;
                        STATE ("movw %s:%s, %s:%s[%02x%02x]\n",
			       avr_regname (d), avr_regname (d + 1), avr_regname (r), avr_regname (r + 1),
			       sram[r + 1], sram[r]);
                        _avr_set_r (avr, d, sram[r]);
                        _avr_set_r (avr, d + 1, sram[r + 1]);
                      }
                      break;
                    case 0x0200:
                      {   // MULS -- Multiply Signed -- 0000 0010 dddd rrrr
                        int8_t r = 16 + (opcode & 0xf);
                        int8_t d = 16 + ((opcode >> 4) & 0xf);
                        int16_t res = ((int8_t) sram[r]) * ((int8_t) sram[d]);
                        STATE ("muls %s[%d], %s[%02x] = %d\n",
			       avr_regname (d), ((int8_t) sram[d]), avr_regname (r),
			       ((int8_t) sram[r]), res);
                        _avr_set_r (avr, 0, res);
                        _avr_set_r (avr, 1, res >> 8);
                        avr->sreg[S_C] = (res >> 15) & 1;
                        avr->sreg[S_Z] = res == 0;
                        cycle++;
                        SREG ();
                      }
                      break;
                    case 0x0300:
                      {   // MUL -- Multiply -- 0000 0011 fddd frrr
                        int8_t r = 16 + (opcode & 0x7);
                        int8_t d = 16 + ((opcode >> 4) & 0x7);
                        int16_t res = 0;
                        uint8_t c = 0;
                        T (const char *name = "";);
			switch (opcode & 0x88)
			  {
			  case 0x00:   // MULSU -- Multiply Signed Unsigned -- 0000 0011 0ddd 0rrr
			    res = ((uint8_t) sram[r]) * ((int8_t) sram[d]);
			    c = (res >> 15) & 1;
			    T (name = "mulsu";);
			    break;
			  case 0x08:   // FMUL -- Fractional Multiply Unsigned -- 0000 0011 0ddd 1rrr
			    res = ((uint8_t) sram[r]) * ((uint8_t) sram[d]);
			    c = (res >> 15) & 1;
			    res <<= 1;
			    T (name = "fmul";);
			    break;
			  case 0x80:   // FMULS -- Multiply Signed -- 0000 0011 1ddd 0rrr
			    res = ((int8_t) sram[r]) * ((int8_t) sram[d]);
			    c = (res >> 15) & 1;
			    res <<= 1;
			    T (name = "fmuls";);
			    break;
			  case 0x88:   // FMULSU -- Multiply Signed Unsigned -- 0000 0011 1ddd 1rrr
			    res = ((uint8_t) sram[r]) * ((int8_t) sram[d]);
			    c = (res >> 15) & 1;
			    res <<= 1;
			    T (name = "fmulsu";);
			    break;
			  }
                        cycle++;
                        STATE ("%s %s[%d], %s[%02x] = %d\n",
			       name, avr_regname (d), ((int8_t) sram[d]), avr_regname (r),
			       ((int8_t) sram[r]), res);
                        _avr_set_r (avr, 0, res);
                        _avr_set_r (avr, 1, res >> 8);
                        avr->sreg[S_C] = c;
                        avr->sreg[S_Z] = res == 0;
                        SREG ();
                      }
                      break;
                    default:
                      _avr_invalid_opcode (avr);
                    }
                }
            }
          }
      }
      break;

    case 0x1000:
      {
        switch (opcode & 0xfc00)
          {
          case 0x1800:
            {   // SUB -- Subtract without carry -- 0001 10rd dddd rrrr
              get_vd5_vr5 (opcode);
              uint8_t res = vd - vr;
              STATE ("sub %s[%02x], %s[%02x] = %02x\n",
		     avr_regname (d), vd, avr_regname (r), vr, res);
              _avr_set_r (avr, d, res);
              _avr_flags_sub_zns (avr, res, vd, vr);
              SREG ();
            }
            break;
          case 0x1000:
            {   // CPSE -- Compare, skip if equal -- 0001 00rd dddd rrrr
              get_vd5_vr5 (opcode);
              uint16_t res = vd == vr;
              STATE ("cpse %s[%02x], %s[%02x]\t; Will%s skip\n",
		     avr_regname (d), sram[d], avr_regname (r), sram[r], res ? "" : " not");
              if (res)
		{
		  if (_avr_is_instruction_32_bits (avr, new_pc))
		    {
		      new_pc += 4;
		      cycle += 2;
		    }
		  else
		    {
		      new_pc += 2;
		      cycle++;
		    }
		}
            }
            break;
          case 0x1400:
            {   // CP -- Compare -- 0001 01rd dddd rrrr
              get_vd5_vr5 (opcode);
              uint8_t res = vd - vr;
              STATE ("cp %s[%02x], %s[%02x] = %02x\n",
		     avr_regname (d), vd, avr_regname (r), vr, res);
              _avr_flags_sub_zns (avr, res, vd, vr);
              SREG ();
            }
            break;
          case 0x1c00:
            {   // ADD -- Add with carry -- 0001 11rd dddd rrrr
              get_vd5_vr5 (opcode);
              uint8_t res = vd + vr + avr->sreg[S_C];
              if (r == d)
		{
		  STATE ("rol %s[%02x] = %02x\n", avr_regname (d), sram[d], res);
		}
              else
		{
		  STATE ("addc %s[%02x], %s[%02x] = %02x\n",
			 avr_regname (d), sram[d], avr_regname (r), sram[r], res);
		}
              _avr_set_r (avr, d, res);
              _avr_flags_add_zns (avr, res, vd, vr);
              SREG ();
            }
            break;
          default:
            _avr_invalid_opcode (avr);
          }
      }
      break;

    case 0x2000:
      {
        switch (opcode & 0xfc00)
          {
          case 0x2000:
            {   // AND -- Logical AND -- 0010 00rd dddd rrrr
              get_vd5_vr5 (opcode);
              uint8_t res = vd & vr;
              if (r == d)
		{
		  STATE ("tst %s[%02x]\n", avr_regname (d), sram[d]);
		}
              else
		{
		  STATE ("and %s[%02x], %s[%02x] = %02x\n", avr_regname (d), vd, avr_regname (r), vr, res);
		}
              _avr_set_r (avr, d, res);
              _avr_flags_znv0s (avr, res);
              SREG ();
            }
            break;
          case 0x2400:
            {   // EOR -- Logical Exclusive OR -- 0010 01rd dddd rrrr
              get_vd5_vr5 (opcode);
              uint8_t res = vd ^ vr;
              if (r == d)
		{
		  STATE ("clr %s[%02x]\n", avr_regname (d), sram[d]);
		}
              else
		{
		  STATE ("eor %s[%02x], %s[%02x] = %02x\n", avr_regname (d), vd, avr_regname (r), vr, res);
		}
              _avr_set_r (avr, d, res);
              _avr_flags_znv0s (avr, res);
              SREG ();
            }
            break;
          case 0x2800:
            {   // OR -- Logical OR -- 0010 10rd dddd rrrr
              get_vd5_vr5 (opcode);
              uint8_t res = vd | vr;
              STATE ("or %s[%02x], %s[%02x] = %02x\n", avr_regname (d), vd, avr_regname (r), vr, res);
              _avr_set_r (avr, d, res);
              _avr_flags_znv0s (avr, res);
              SREG ();
            }
            break;
          case 0x2c00:
            {   // MOV -- 0010 11rd dddd rrrr
              get_d5_vr5 (opcode);
              uint8_t res = vr;
              STATE ("mov %s, %s[%02x] = %02x\n", avr_regname (d), avr_regname (r), vr, res);
              _avr_set_r (avr, d, res);
            }
            break;
          default:
            _avr_invalid_opcode (avr);
          }
      }
      break;

    case 0x3000:
      {   // CPI -- Compare Immediate -- 0011 kkkk hhhh kkkk
        get_vh4_k8 (opcode);
        uint8_t res = vh - k;
        STATE ("cpi %s[%02x], 0x%02x\n", avr_regname (h), vh, k);
        _avr_flags_sub_zns (avr, res, vh, k);
        SREG ();
      }
      break;

    case 0x4000:
      {   // SBCI -- Subtract Immediate With Carry -- 0100 kkkk hhhh kkkk
        get_vh4_k8 (opcode);
        uint8_t res = vh - k - avr->sreg[S_C];
        STATE ("sbci %s[%02x], 0x%02x = %02x\n", avr_regname (h), vh, k, res);
        _avr_set_r (avr, h, res);
        _avr_flags_sub_Rzns (avr, res, vh, k);
        SREG ();
      }
      break;

    case 0x5000:
      {   // SUBI -- Subtract Immediate -- 0101 kkkk hhhh kkkk
        get_vh4_k8 (opcode);
        uint8_t res = vh - k;
        STATE ("subi %s[%02x], 0x%02x = %02x\n", avr_regname (h), vh, k, res);
        _avr_set_r (avr, h, res);
        _avr_flags_sub_zns (avr, res, vh, k);
        SREG ();
      }
      break;

    case 0x6000:
      {   // ORI aka SBR -- Logical OR with Immediate -- 0110 kkkk hhhh kkkk
        get_vh4_k8 (opcode);
        uint8_t res = vh | k;
        STATE ("ori %s[%02x], 0x%02x\n", avr_regname (h), vh, k);
        _avr_set_r (avr, h, res);
        _avr_flags_znv0s (avr, res);
        SREG ();
      }
      break;

    case 0x7000:
      {   // ANDI -- Logical AND with Immediate -- 0111 kkkk hhhh kkkk
        get_vh4_k8 (opcode);
        uint8_t res = vh & k;
        STATE ("andi %s[%02x], 0x%02x\n", avr_regname (h), vh, k);
        _avr_set_r (avr, h, res);
        _avr_flags_znv0s (avr, res);
        SREG ();
      }
      break;

    case 0xa000:
    case 0x8000:
      {
        /*
         * Load (LDD/STD) store instructions
         *
         * 10q0 qqsd dddd yqqq
         * s = 0 = load, 1 = store
         * y = 16 bits register index, 1 = Y, 0 = X
         * q = 6 bit displacement
         */
        switch (opcode & 0xd008)
          {
          case 0xa000:
          case 0x8000:
            {   // LD (LDD) -- Load Indirect using Z -- 10q0 qqsd dddd yqqq
              uint16_t v = read_uint16_lh(sram, R_ZL, R_ZH);
              get_d5_q6 (opcode);
              if (opcode & 0x0200)
                {
                  STATE ("st (Z+%d[%04x]), %s[%02x]\n", q, v + q, avr_regname (d), sram[d]);
                  _avr_set_ram (avr, v + q, sram[d]);
                }
              else
                {
                  STATE ("ld %s, (Z+%d[%04x])=[%02x]\n", avr_regname (d), q, v + q, sram[v + q]);
                  _avr_set_r (avr, d, _avr_get_ram (avr, v + q));
                }
              cycle += 1;   // 2 cycles, 3 for tinyavr
            }
            break;
          case 0xa008:
          case 0x8008:
            {   // LD (LDD) -- Load Indirect using Y -- 10q0 qqsd dddd yqqq
              uint16_t v = read_uint16_lh(sram, R_YL, R_YH);
              get_d5_q6 (opcode);
              if (opcode & 0x0200)
                {
                  STATE ("st (Y+%d[%04x]), %s[%02x]\n", q, v + q, avr_regname (d), sram[d]);
                  _avr_set_ram (avr, v + q, sram[d]);
                }
              else
                {
                  STATE ("ld %s, (Y+%d[%04x])=[%02x]\n", avr_regname (d), q, v + q, sram[d + q]);
                  _avr_set_r (avr, d, _avr_get_ram (avr, v + q));
                }
              cycle += 1;   // 2 cycles, 3 for tinyavr
            }
            break;
          default:
            _avr_invalid_opcode (avr);
          }
      }
      break;

    case 0x9000:
      {
        /* this is an annoying special case, but at least these lines handle all the SREG set/clear opcodes */
        if ((opcode & 0xff0f) == 0x9408)
          {
            get_sreg_bit (opcode);
            STATE ("%s%c\n", opcode & 0x0080 ? "cl" : "se", _sreg_bit_name[b]);
            avr_sreg_set (avr, b, (opcode & 0x0080) == 0);
            SREG ();
          }
        else
          switch (opcode)
            {
            case 0x9588:
              {   // SLEEP -- 1001 0101 1000 1000
                STATE ("sleep\n");
                /* Don't sleep if there are interrupts about to be serviced.
                 * Without this check, it was possible to incorrectly enter a state
                 * in which the cpu was sleeping and interrupts were disabled. For more
                 * details, see the commit message. */
                if (!avr_has_pending_interrupts (avr) || !avr->sreg[S_I])
                  avr->state = cpu_Sleeping;
              }
              break;
            case 0x9598:
              {   // BREAK -- 1001 0101 1001 1000
                STATE ("break\n");
                if (avr->gdb)
                  {
		    // if gdb is on, we break here as in here and we do so until gdb restores the
		    // instruction that was here before
                    avr->state = cpu_StepDone;
                    new_pc = avr->pc;
                    cycle = 0;
                  }
              }
              break;
            case 0x95a8:
              {   // WDR -- Watchdog Reset -- 1001 0101 1010 1000
                STATE ("wdr\n");
                avr_ioctl (avr, AVR_IOCTL_WATCHDOG_RESET, 0);
              }
              break;
            case 0x95e8:
              {   // SPM -- Store Program Memory -- 1001 0101 1110 1000
                STATE ("spm\n");
                avr_ioctl (avr, AVR_IOCTL_FLASH_SPM, 0);
              }
              break;
            case 0x9409:   // IJMP -- Indirect jump -- 1001 0100 0000 1001
            case 0x9419:   // EIJMP -- Indirect jump -- 1001 0100 0001 1001   bit 4 is "indirect"
            case 0x9509:   // ICALL -- Indirect Call to Subroutine -- 1001 0101 0000 1001
            case 0x9519:
              {   // EICALL -- Indirect Call to Subroutine -- 1001 0101 0001 1001   bit 8 is "push pc"
                int e = opcode & 0x10;
                int p = opcode & 0x100;
                if (e && !avr->eind)
                  _avr_invalid_opcode (avr);
                uint32_t z = read_uint16_lh(sram, R_ZL, R_ZH);
                if (e)
                  z |= sram[avr->eind] << 16;
                STATE ("%si%s Z[%04x]\n", e ? "e" : "", p ? "call" : "jmp", z << 1);
                if (p)
                  cycle += _avr_push_addr (avr, new_pc) - 1;
                new_pc = z << 1;
                cycle++;
                TRACE_JUMP ();
              }
              break;
            case 0x9518:   // RETI -- Return from Interrupt -- 1001 0101 0001 1000
            case 0x9508:
              {   // RET -- Return -- 1001 0101 0000 1000
                new_pc = _avr_pop_addr (avr);
                cycle += 1 + avr->address_size;
                if (opcode & 0x10)   // reti
                  avr_sreg_set (avr, S_I, 1);
                STATE ("ret%s\n", opcode & 0x10 ? "i" : "");
                TRACE_JUMP ();
                STACK_FRAME_POP ();
              }
              break;
            case 0x95c8:
              {   // LPM -- Load Program Memory R0 <- (Z) -- 1001 0101 1100 1000
                uint16_t z = read_uint16_lh(sram, R_ZL, R_ZH);
                STATE ("lpm %s, (Z[%04x])\n", avr_regname (0), z);
                cycle += 2;   // 3 cycles
                _avr_set_r (avr, 0, avr->flash[z]);
              }
              break;
            default:
              {
                switch (opcode & 0xfe0f)
                  {
                  case 0x9000:
                    {   // LDS -- Load Direct from Data Space, 32 bits -- 1001 0000 0000 0000
                      get_d5 (opcode);
                      uint16_t x = _avr_read_instruction(avr, new_pc);
                      new_pc += 2;
                      STATE ("lds %s[%02x], 0x%04x\n", avr_regname (d), sram[d], x);
                      _avr_set_r (avr, d, _avr_get_ram (avr, x));
                      cycle++;   // 2 cycles
                    }
                    break;
                  case 0x9005:
                  case 0x9004:
                    {   // LPM -- Load Program Memory -- 1001 000d dddd 01oo
                      get_d5 (opcode);
                      uint16_t z = read_uint16_lh(sram, R_ZL, R_ZH);
                      int op = opcode & 1;
                      STATE ("lpm %s, (Z[%04x]%s)\n", avr_regname (d), z, op ? "+" : "");
                      _avr_set_r (avr, d, avr->flash[z]);
                      if (op)
                        {
                          z++;
                          _avr_set_r (avr, R_ZH, z >> 8);
                          _avr_set_r (avr, R_ZL, z);
                        }
                      cycle += 2;   // 3 cycles
                    }
                    break;
                  case 0x9006:
                  case 0x9007:
                    {   // ELPM -- Extended Load Program Memory -- 1001 000d dddd 01oo
                      if (!avr->rampz)
                        _avr_invalid_opcode (avr);
                      uint32_t z = read_uint16_lh(sram, R_ZL, R_ZH) | (sram[avr->rampz] << 16);
                      get_d5 (opcode);
                      int op = opcode & 1;
                      STATE ("elpm %s, (Z[%02x:%04x]%s)\n", avr_regname (d), z >> 16, z & 0xffff, op ? "+" : "");
                      _avr_set_r (avr, d, avr->flash[z]);
                      if (op)
                        {
                          z++;
                          _avr_set_r (avr, avr->rampz, z >> 16);
                          _avr_set_r (avr, R_ZH, z >> 8);
                          _avr_set_r (avr, R_ZL, z);
                        }
                      cycle += 2;   // 3 cycles
                    }
                    break;
                    /*
                     * Load store instructions
                     *
                     * 1001 00sr rrrr iioo
                     * s = 0 = load, 1 = store
                     * ii = 16 bits register index, 11 = X, 10 = Y, 00 = Z
                     * oo = 1) post increment, 2) pre-decrement
                     */
                  case 0x900c:
                  case 0x900d:
                  case 0x900e:
                    {   // LD -- Load Indirect from Data using X -- 1001 000d dddd 11oo
                      int op = opcode & 3;
                      get_d5 (opcode);
                      uint16_t x = read_uint16_lh(sram, R_XL, R_XH);
                      STATE ("ld %s, %sX[%04x]%s\n",
			     avr_regname (d), op == 2 ? "--" : "", x, op == 1 ? "++" : "");
                      cycle++;   // 2 cycles (1 for tinyavr, except with inc/dec 2)
                      if (op == 2)
                        x--;
                      uint8_t vd = _avr_get_ram (avr, x);
                      if (op == 1)
                        x++;
                      _avr_set_r (avr, R_XH, x >> 8);
                      _avr_set_r (avr, R_XL, x);
                      _avr_set_r (avr, d, vd);
                    }
                    break;
                  case 0x920c:
                  case 0x920d:
                  case 0x920e:
                    {   // ST -- Store Indirect Data Space X -- 1001 001d dddd 11oo
                      int op = opcode & 3;
                      get_vd5 (opcode);
                      uint16_t x = read_uint16_lh(sram, R_XL, R_XH);
                      STATE ("st %sX[%04x]%s, %s[%02x] \n",
			     op == 2 ? "--" : "", x, op == 1 ? "++" : "", avr_regname (d), vd);
                      cycle++;   // 2 cycles, except tinyavr
                      if (op == 2)
                        x--;
                      _avr_set_ram (avr, x, vd);
                      if (op == 1)
                        x++;
                      _avr_set_r (avr, R_XH, x >> 8);
                      _avr_set_r (avr, R_XL, x);
                    }
                    break;
                  case 0x9009:
                  case 0x900a:
                    {   // LD -- Load Indirect from Data using Y -- 1001 000d dddd 10oo
                      int op = opcode & 3;
                      get_d5 (opcode);
                      uint16_t y = read_uint16_lh(sram, R_YL, R_YH);
                      STATE ("ld %s, %sY[%04x]%s\n",
			     avr_regname (d), op == 2 ? "--" : "", y, op == 1 ? "++" : "");
                      cycle++;   // 2 cycles, except tinyavr
                      if (op == 2)
                        y--;
                      uint8_t vd = _avr_get_ram (avr, y);
                      if (op == 1)
                        y++;
                      _avr_set_r (avr, R_YH, y >> 8);
                      _avr_set_r (avr, R_YL, y);
                      _avr_set_r (avr, d, vd);
                    }
                    break;
                  case 0x9209:
                  case 0x920a:
                    {   // ST -- Store Indirect Data Space Y -- 1001 001d dddd 10oo
                      int op = opcode & 3;
                      get_vd5 (opcode);
                      uint16_t y = read_uint16_lh(sram, R_YL, R_YH);
                      STATE ("st %sY[%04x]%s, %s[%02x]\n",
			     op == 2 ? "--" : "", y, op == 1 ? "++" : "", avr_regname (d), vd);
                      cycle++;
                      if (op == 2)
                        y--;
                      _avr_set_ram (avr, y, vd);
                      if (op == 1)
                        y++;
                      _avr_set_r (avr, R_YH, y >> 8);
                      _avr_set_r (avr, R_YL, y);
                    }
                    break;
                  case 0x9200:
                    {   // STS -- Store Direct to Data Space, 32 bits -- 1001 0010 0000 0000
                      get_vd5 (opcode);
                      uint16_t x = _avr_read_instruction(avr, new_pc);
                      new_pc += 2;
                      STATE ("sts 0x%04x, %s[%02x]\n", x, avr_regname (d), vd);
                      cycle++;
                      _avr_set_ram (avr, x, vd);
                    }
                    break;
                  case 0x9001:
                  case 0x9002:
                    {   // LD -- Load Indirect from Data using Z -- 1001 000d dddd 00oo
                      int op = opcode & 3;
                      get_d5 (opcode);
                      uint16_t z = read_uint16_lh(sram, R_ZL, R_ZH);
                      STATE ("ld %s, %sZ[%04x]%s\n",
			     avr_regname (d), op == 2 ? "--" : "", z, op == 1 ? "++" : "");
                      cycle++;;   // 2 cycles, except tinyavr
                      if (op == 2)
                        z--;
                      uint8_t vd = _avr_get_ram (avr, z);
                      if (op == 1)
                        z++;
                      _avr_set_r (avr, R_ZH, z >> 8);
                      _avr_set_r (avr, R_ZL, z);
                      _avr_set_r (avr, d, vd);
                    }
                    break;
                  case 0x9201:
                  case 0x9202:
                    {   // ST -- Store Indirect Data Space Z -- 1001 001d dddd 00oo
                      int op = opcode & 3;
                      get_vd5 (opcode);
                      uint16_t z = read_uint16_lh(sram, R_ZL, R_ZH);
                      STATE ("st %sZ[%04x]%s, %s[%02x] \n",
			     op == 2 ? "--" : "", z, op == 1 ? "++" : "", avr_regname (d), vd);
                      cycle++;   // 2 cycles, except tinyavr
                      if (op == 2)
                        z--;
                      _avr_set_ram (avr, z, vd);
                      if (op == 1)
                        z++;
                      _avr_set_r (avr, R_ZH, z >> 8);
                      _avr_set_r (avr, R_ZL, z);
                    }
                    break;
                  case 0x900f:
                    {   // POP -- 1001 000d dddd 1111
                      get_d5 (opcode);
                      _avr_set_r (avr, d, _avr_pop8 (avr));
                      T (uint16_t sp = _avr_sp_get (avr););
		      STATE ("pop %s (@%04x)[%02x]\n", avr_regname (d), sp, sram[sp]);
                      cycle++;
                    }
                    break;
                  case 0x920f:
                    {   // PUSH -- 1001 001d dddd 1111
                      get_vd5 (opcode);
                      _avr_push8 (avr, vd);
                      T (uint16_t sp = _avr_sp_get (avr););
		      STATE ("push %s[%02x] (@%04x)\n", avr_regname (d), vd, sp);
                      cycle++;
                    }
                    break;
                  case 0x9400:
                    {   // COM -- One’s Complement -- 1001 010d dddd 0000
                      get_vd5 (opcode);
                      uint8_t res = 0xff - vd;
                      STATE ("com %s[%02x] = %02x\n", avr_regname (d), vd, res);
                      _avr_set_r (avr, d, res);
                      _avr_flags_znv0s (avr, res);
                      avr->sreg[S_C] = 1;
                      SREG ();
                    }
                    break;
                  case 0x9401:
                    {   // NEG -- Two’s Complement -- 1001 010d dddd 0001
                      get_vd5 (opcode);
                      uint8_t res = 0x00 - vd;
                      STATE ("neg %s[%02x] = %02x\n", avr_regname (d), vd, res);
                      _avr_set_r (avr, d, res);
                      avr->sreg[S_H] = ((res >> 3) | (vd >> 3)) & 1;
                      avr->sreg[S_V] = res == 0x80;
                      avr->sreg[S_C] = res != 0;
                      _avr_flags_zns (avr, res);
                      SREG ();
                    }
                    break;
                  case 0x9402:
                    {   // SWAP -- Swap Nibbles -- 1001 010d dddd 0010
                      get_vd5 (opcode);
                      uint8_t res = (vd >> 4) | (vd << 4);
                      STATE ("swap %s[%02x] = %02x\n", avr_regname (d), vd, res);
                      _avr_set_r (avr, d, res);
                    }
                    break;
                  case 0x9403:
                    {   // INC -- Increment -- 1001 010d dddd 0011
                      get_vd5 (opcode);
                      uint8_t res = vd + 1;
                      STATE ("inc %s[%02x] = %02x\n", avr_regname (d), vd, res);
                      _avr_set_r (avr, d, res);
                      avr->sreg[S_V] = res == 0x80;
                      _avr_flags_zns (avr, res);
                      SREG ();
                    }
                    break;
                  case 0x9405:
                    {   // ASR -- Arithmetic Shift Right -- 1001 010d dddd 0101
                      get_vd5 (opcode);
                      uint8_t res = (vd >> 1) | (vd & 0x80);
                      STATE ("asr %s[%02x]\n", avr_regname (d), vd);
                      _avr_set_r (avr, d, res);
                      _avr_flags_zcnvs (avr, res, vd);
                      SREG ();
                    }
                    break;
                  case 0x9406:
                    {   // LSR -- Logical Shift Right -- 1001 010d dddd 0110
                      get_vd5 (opcode);
                      uint8_t res = vd >> 1;
                      STATE ("lsr %s[%02x]\n", avr_regname (d), vd);
                      _avr_set_r (avr, d, res);
                      avr->sreg[S_N] = 0;
                      _avr_flags_zcvs (avr, res, vd);
                      SREG ();
                    }
                    break;
                  case 0x9407:
                    {   // ROR -- Rotate Right -- 1001 010d dddd 0111
                      get_vd5 (opcode);
                      uint8_t res = (avr->sreg[S_C] ? 0x80 : 0) | vd >> 1;
                      STATE ("ror %s[%02x]\n", avr_regname (d), vd);
                      _avr_set_r (avr, d, res);
                      _avr_flags_zcnvs (avr, res, vd);
                      SREG ();
                    }
                    break;
                  case 0x940a:
                    {   // DEC -- Decrement -- 1001 010d dddd 1010
                      get_vd5 (opcode);
                      uint8_t res = vd - 1;
                      STATE ("dec %s[%02x] = %02x\n", avr_regname (d), vd, res);
                      _avr_set_r (avr, d, res);
                      avr->sreg[S_V] = res == 0x7f;
                      _avr_flags_zns (avr, res);
                      SREG ();
                    }
                    break;
                  case 0x940c:
                  case 0x940d:
                    {   // JMP -- Long Call to sub, 32 bits -- 1001 010a aaaa 110a
                      avr_flashaddr_t a = ((opcode & 0x01f0) >> 3) | (opcode & 1);
                      uint16_t x = _avr_read_instruction(avr, new_pc);
                      a = (a << 16) | x;
                      STATE ("jmp 0x%06x\n", a);
                      new_pc = a << 1;
                      cycle += 2;
                      TRACE_JUMP ();
                    }
                    break;
                  case 0x940e:
                  case 0x940f:
                    {   // CALL -- Long Call to sub, 32 bits -- 1001 010a aaaa 111a
                      avr_flashaddr_t a = ((opcode & 0x01f0) >> 3) | (opcode & 1);
                      uint16_t x = _avr_read_instruction(avr, new_pc);
                      a = (a << 16) | x;
                      STATE ("call 0x%06x\n", a);
                      new_pc += 2;
                      cycle += 1 + _avr_push_addr (avr, new_pc);
                      new_pc = a << 1;
                      TRACE_JUMP ();
                      STACK_FRAME_PUSH ();
                    }
                    break;

                  default:
                    {
                      switch (opcode & 0xff00)
                        {
                        case 0x9600:
                          {   // ADIW -- Add Immediate to Word -- 1001 0110 KKpp KKKK
                            get_vp2_k6 (opcode);
                            uint16_t res = vp + k;
			    STATE ("adiw %s:%s[%04x], 0x%02x\n",
				   avr_regname (p), avr_regname (p + 1), vp, k);
                            _avr_set_r (avr, p + 1, res >> 8);
                            _avr_set_r (avr, p, res);
                            avr->sreg[S_V] = ((~vp & res) >> 15) & 1;
                            avr->sreg[S_C] = ((~res & vp) >> 15) & 1;
                            _avr_flags_zns16 (avr, res);
                            SREG ();
                            cycle++;
                          }
                          break;
                        case 0x9700:
                          {   // SBIW -- Subtract Immediate from Word -- 1001 0111 KKpp KKKK
                            get_vp2_k6 (opcode);
                            uint16_t res = vp - k;
                            STATE ("sbiw %s:%s[%04x], 0x%02x\n",
				   avr_regname (p), avr_regname (p + 1), vp, k);
                            _avr_set_r (avr, p + 1, res >> 8);
                            _avr_set_r (avr, p, res);
                            avr->sreg[S_V] = ((vp & ~res) >> 15) & 1;
                            avr->sreg[S_C] = ((res & ~vp) >> 15) & 1;
                            _avr_flags_zns16 (avr, res);
                            SREG ();
                            cycle++;
                          }
                          break;
                        case 0x9800:
                          {   // CBI -- Clear Bit in I/O Register -- 1001 1000 AAAA Abbb
                            get_io5_b3mask (opcode);
                            uint8_t res = _avr_get_ram (avr, io) & ~mask;
			    STATE ("cbi %s[%04x], 0x%02x = %02x\n",
				   avr_regname (io), sram[io], mask, res);
                            _avr_set_ram (avr, io, res);
                            cycle++;
                          }
                          break;
                        case 0x9900:
                          {   // SBIC -- Skip if Bit in I/O Register is Cleared -- 1001 1001 AAAA Abbb
                            get_io5_b3mask (opcode);
                            uint8_t res = _avr_get_ram (avr, io) & mask;
                            STATE ("sbic %s[%04x], 0x%02x\t; Will%s branch\n",
				   avr_regname (io), sram[io], mask, !res ? "" : " not");
                            if (!res)
                              {
                                if (_avr_is_instruction_32_bits (avr, new_pc))
                                  {
                                    new_pc += 4;
                                    cycle += 2;
                                  }
                                else
                                  {
                                    new_pc += 2;
                                    cycle++;
                                  }
                              }
                          }
                          break;
                        case 0x9a00:
                          {   // SBI -- Set Bit in I/O Register -- 1001 1010 AAAA Abbb
                            get_io5_b3mask (opcode);
                            uint8_t res = _avr_get_ram (avr, io) | mask;
                            STATE ("sbi %s[%04x], 0x%02x = %02x\n", avr_regname (io), sram[io],
                                   mask, res);
                            _avr_set_ram (avr, io, res);
                            cycle++;
                          }
                          break;
                        case 0x9b00:
                          {   // SBIS -- Skip if Bit in I/O Register is Set -- 1001 1011 AAAA Abbb
                            get_io5_b3mask (opcode);
                            uint8_t res = _avr_get_ram (avr, io) & mask;
                            STATE ("sbis %s[%04x], 0x%02x\t; Will%s branch\n",
				   avr_regname (io), sram[io], mask, res ? "" : " not");
                            if (res)
			      {
				if (_avr_is_instruction_32_bits (avr, new_pc))
				  {
				    new_pc += 4;
				    cycle += 2;
				  }
				else
				  {
				    new_pc += 2;
				    cycle++;
				  }
			      }
                          }
                          break;
                        default:
                          switch (opcode & 0xfc00)
                            {
                            case 0x9c00:
                              {   // MUL -- Multiply Unsigned -- 1001 11rd dddd rrrr
                                get_vd5_vr5 (opcode);
                                uint16_t res = vd * vr;
                                STATE ("mul %s[%02x], %s[%02x] = %04x\n",
				       avr_regname (d), vd, avr_regname (r), vr, res);
                                cycle++;
                                _avr_set_r (avr, 0, res);
                                _avr_set_r (avr, 1, res >> 8);
                                avr->sreg[S_Z] = res == 0;
                                avr->sreg[S_C] = (res >> 15) & 1;
                                SREG ();
                              }
                              break;
                            default:
                              _avr_invalid_opcode (avr);
                            }
                        }
                    }
                    break;
                  }
              }
              break;
            }
      }
      break;

    case 0xb000:
      {
        switch (opcode & 0xf800)
          {
          case 0xb800:
            {   // OUT A,Rr -- 1011 1AAd dddd AAAA
              get_d5_a6 (opcode);
              STATE ("out %s, %s[%02x]\n", avr_regname (A), avr_regname (d), sram[d]);
              _avr_set_ram (avr, A, sram[d]);
            }
            break;
          case 0xb000:
            {   // IN Rd,A -- 1011 0AAd dddd AAAA
              get_d5_a6 (opcode);
              STATE ("in %s, %s[%02x]\n", avr_regname (d), avr_regname (A), sram[A]);
              _avr_set_r (avr, d, _avr_get_ram (avr, A));
            }
            break;
          default:
            _avr_invalid_opcode (avr);
          }
      }
      break;

    case 0xc000:
      {   // RJMP -- 1100 kkkk kkkk kkkk
        get_o12 (opcode);
        STATE ("rjmp .%d [%04x]\n", o >> 1, new_pc + o);
        new_pc = new_pc + o;
        cycle++;
        TRACE_JUMP ();
      }
      break;

    case 0xd000:
      {   // RCALL -- 1101 kkkk kkkk kkkk
        get_o12 (opcode);
        STATE ("rcall .%d [%04x]\n", o >> 1, new_pc + o);
        cycle += _avr_push_addr (avr, new_pc);
        new_pc = new_pc + o;
	// 'rcall .1' is used as a cheap "push 16 bits of room on the stack"
        if (o != 0)
          {
            TRACE_JUMP ();
            STACK_FRAME_PUSH ();
          }
      }
      break;

    case 0xe000:
      {   // LDI Rd, K aka SER (LDI r, 0xff) -- 1110 kkkk dddd kkkk
        get_h4_k8 (opcode);
        STATE ("ldi %s, 0x%02x\n", avr_regname (h), k);
        _avr_set_r (avr, h, k);
      }
      break;

    case 0xf000:
      {
        switch (opcode & 0xfe00)
          {
          case 0xf000:
          case 0xf200:
          case 0xf400:
          case 0xf600:
            {   // BRXC/BRXS -- All the SREG branches -- 1111 0Boo oooo osss
              int16_t o = ((int16_t) (opcode << 6)) >> 9;   // offset
              uint8_t s = opcode & 7;
              int set = (opcode & 0x0400) == 0;   // this bit means BRXC otherwise BRXS
              int branch = (avr->sreg[s] && set) || (!avr->sreg[s] && !set);
              const char *names[2][8] = {
                {"brcc", "brne", "brpl", "brvc", NULL, "brhc", "brtc", "brid"},
                {"brcs", "breq", "brmi", "brvs", NULL, "brhs", "brts", "brie"},
              };
              if (names[set][s])
		{
		  STATE ("%s .%d [%04x]\t; Will%s branch\n",
			 names[set][s], o, new_pc + (o << 1), branch ? "" : " not");
		}
              else
		{
		  STATE ("%s%c .%d [%04x]\t; Will%s branch\n",
			 set ? "brbs" : "brbc", _sreg_bit_name[s], o, new_pc + (o << 1), branch ? "" : " not");
		}
              if (branch)
                {
                  cycle++;   // 2 cycles if taken, 1 otherwise
                  new_pc = new_pc + (o << 1);
                }
            }
            break;
          case 0xf800:
          case 0xf900:
            {   // BLD -- Bit Store from T into a Bit in Register -- 1111 100d dddd 0bbb
              get_vd5_s3_mask (opcode);
              uint8_t v = (vd & ~mask) | (avr->sreg[S_T] ? mask : 0);
              STATE ("bld %s[%02x], 0x%02x = %02x\n", avr_regname (d), vd, mask, v);
              _avr_set_r (avr, d, v);
            }
            break;
          case 0xfa00:
          case 0xfb00:
            {   // BST -- Bit Store into T from bit in Register -- 1111 101d dddd 0bbb
              get_vd5_s3 (opcode) STATE ("bst %s[%02x], 0x%02x\n", avr_regname (d), vd, 1 << s);
              avr->sreg[S_T] = (vd >> s) & 1;
              SREG ();
            }
            break;
          case 0xfc00:
          case 0xfe00:
            {   // SBRS/SBRC -- Skip if Bit in Register is Set/Clear -- 1111 11sd dddd 0bbb
              get_vd5_s3_mask (opcode) int set = (opcode & 0x0200) != 0;
              int branch = ((vd & mask) && set) || (!(vd & mask) && !set);
              STATE ("%s %s[%02x], 0x%02x\t; Will%s branch\n",
		     set ? "sbrs" : "sbrc", avr_regname (d), vd, mask, branch ? "" : " not");
              if (branch)
		{
		  if (_avr_is_instruction_32_bits (avr, new_pc))
		    {
		      new_pc += 4;
		      cycle += 2;
		    }
		  else
		    {
		      new_pc += 2;
		      cycle++;
		    }
		}
            }
            break;
          default:
            _avr_invalid_opcode (avr);
          }
      }
      break;

    default:
      _avr_invalid_opcode (avr);
    }
  
  avr->cycle += cycle;

  if ((avr->state == cpu_Running) &&
      (avr->run_cycle_count > cycle) &&
      (avr->interrupt_state == 0))
    {
      avr->run_cycle_count -= cycle;
      avr->pc = new_pc;
      goto run_one_again;
    }

  return new_pc;
}
