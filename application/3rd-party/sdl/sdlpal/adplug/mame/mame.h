/*
 * SDLPAL
 * Copyright (c) 2011-2022, SDLPAL development team.
 * All rights reserved.
 *
 * This file is part of SDLPAL.
 *
 * SDLPAL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * mame.h - Adaptered for SDLPAL by Lou Yihua.
 *
 */

#ifndef SDLPAL_MAME_H
#define SDLPAL_MAME_H

#include <stdint.h>
#include <math.h>
#include <string.h>

#undef __WIN_STRING_DEF_MSC_VER
#if defined(__WIN__) && !defined(_MSC_VER)
#   define _MSC_VER    1926
#   define __WIN_STRING_DEF_MSC_VER
#endif

#include <algorithm>

#ifdef __WIN_STRING_DEF_MSC_VER
#   undef __WIN_STRING_DEF_MSC_VER
#   undef _MSC_VER
#endif

typedef void device_t;
struct attotime {};

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

#define MAME_BIT( _INPUT_, _BIT_ ) ( ( _INPUT_) >> (_BIT_)) & 1

#endif
