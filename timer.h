/*
 * Copyright (c) 2009, 2010 by Paul Schmid
 * Copyright (c) 2007 by Hartmut Birr
 *
 * This program is free software; you can redistribute it and/or
 * mmodify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef __TIMER_H__
#define __TIMER_H__

#include <inttypes.h>

#define TIMER_4MS       4
#define TIMER_10MS      10
#define TIMER_50MS      50
#define TIMER_100MS     100

uint8_t TestAndResetTimerOV(uint8_t TimerId);
void InitTimer(void);
uint32_t GetTicker(void);
void wait_us(uint32_t);
void StartTimers(void);

#endif
