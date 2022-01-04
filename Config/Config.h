/*
 * Copyright (c) 2007 by Hartmut Birr
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

// ******************************************************************************************************************
// *** user configurable parameters *********************************************************************************
// ******************************************************************************************************************
// The following defines decide which default values are loaded (12/16bit DAC/ADC hardware version)

//#define DEFAULT_PARAMS_12BIT
#define DEFAULT_PARAMS_16BIT

// DCP module presence selection is actually not required since relay switching is done anyway.
// BTW: OPT 167 Bit2 is ignored.

// Extension board with two 16-Bit DACs to get rid off the sample & hold circuit
//#define DUAL_DAC        // disable this #define if you have just a standard DCG with single DAC

// for mandatory addressing on bus commands for improving bus reliability
#define STRICTSYNTAX    // disable this #define if you want the command parser behavior of original pascal firmware

// ******************************************************************************************************************
// *** internal debugging settings **********************************************************************************
// ******************************************************************************************************************

//#define DEBUGSTDHW    // in case of debugging standard hardware on DUAL-DAC hardware
//#define PRELOAD_ARB_RAM   // for testing arbitrary function

// ******************************************************************************************************************
// *** version strings **********************************************************************************************
// ******************************************************************************************************************

#define VERSSTRLONG_DUAL    "1.3 [DCG2a by HB + PSC + KSCH]"
#define VERSSTRLONG_SINGLE  "1.3 [DCG2a by HB + PSC + KSCH]"

#define VERSSTRSHORT_DUAL   "DCG2a1.3"
#define VERSSTRSHORT_SINGLE "DCG2a1.3"

// **********************************************************************************************

// checking user settings
#if defined DEFAULT_PARAMS_12BIT && defined DEFAULT_PARAMS_16BIT
#error "Configuration error: only define either 12bit or 16bit hardware default parameters"
#endif

#if ! defined DEFAULT_PARAMS_12BIT && ! defined DEFAULT_PARAMS_16BIT
#error "Configuration error: define either 12bit or 16bit hardware default parameters"
#endif

// Dual DAC option is only supported on 16bit version
#if defined DEFAULT_PARAMS_12BIT && defined DUAL_DAC
#undef DUAL_DAC
#endif


#endif
