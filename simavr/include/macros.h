/*
 * macros.h
 *
 * Copyright 2015 Fabrice Salvaire <fabrice.salvaire@orange.fr>
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

/**
 * @defgroup macros Macros
 * @{
 */

#include <stdint.h>

#ifndef __MACROS_H__
#define __MACROS_H__

#ifdef __cplusplus
extern "C"
{
#endif

/// Test a bit
#define IS_BIT_SET(value, i) (value & (1 << i))

// Return the bit's value
#define BIT_VALUE(value, i) ((value >> i) & 1)

// #define MaskedValue(value1, value2, i) (value1 & ~mask) | (value2 & mask)

static inline uint16_t
read_uint16(uint8_t *base, size_t offset)
{
  return (base[offset + 1] << 8) | base[offset];
}

static inline uint16_t
read_uint16_lh(uint8_t *base, size_t low_offset, size_t high_offset)
{
  return (base[high_offset] << 8) | base[low_offset];
}
  
#ifdef __cplusplus
};
#endif

#endif /*__MACROS_H__*/
/// @} end of macros group
