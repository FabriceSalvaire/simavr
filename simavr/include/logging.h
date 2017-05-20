/*
 * logging.h
 *
 * Copyright 2015 Michel Pollet <buserror@gmail.com>
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
 * @defgroup logging Logging
 * @{
 */

#ifndef __LOGGING_H__
#define __LOGGING_H__

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef NO_COLOR
#define FONT_GREEN
#define FONT_RED
#define FONT_DEFAULT
#else
#define FONT_GREEN  "\e[32m"
#define FONT_RED  "\e[31m"
#define FONT_DEFAULT "\e[0m"
#endif

#ifdef __cplusplus
};
#endif

#endif /*__LOGGING_H__*/
/// @} end of logging group
