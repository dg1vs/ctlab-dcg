/*
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


#include "avr/io.h"
#include "avr/interrupt.h"
#include "config.h"

.global ShiftIn1864
.func ShiftIn1864
ShiftIn1864:

    push    r20
    sbi     _SFR_IO_ADDR(PORTB), PB0

    ldi     r20, 0x15                   ; 1
.L1:
    dec     r20                         ; 1
    brne    .L1                         ; 1/2
                                        ; 63 = 3.9us at 16MHz

    cbi     _SFR_IO_ADDR(PORTB), PB7
    ldi     r20, 8
    clr     r25
.L2:
    cbi     _SFR_IO_ADDR(PORTB), PB0
    lsl     r25
    sbi     _SFR_IO_ADDR(PORTB), PB0
    sbic    _SFR_IO_ADDR(PINB), PB6	
    ori     r25, 1
    dec     r20
    brne    .L2

    ldi     r20, 8
    clr     r24
.L3:
    cbi     _SFR_IO_ADDR(PORTB), PB0
    lsl     r24
    sbi     _SFR_IO_ADDR(PORTB), PB0
    sbic    _SFR_IO_ADDR(PINB), PB6	
    ori     r24, 1
    dec     r20
    brne    .L3

    pop     r20
    sbi     _SFR_IO_ADDR(PORTB), PB7
    ret

.endfunc

.global ShiftOut1655
.func ShiftOut1655
ShiftOut1655:

    push    r20
    push    r25

    cbi     _SFR_IO_ADDR(PORTB), PB0

#ifdef DUAL_DAC	
	tst 	r22
	breq    DAC_U1
DAC_I1:
	cbi 	_SFR_IO_ADDR(PORTC), PC5
	jmp 	DAC_1
DAC_U1:
	cbi		_SFR_IO_ADDR(PORTC), PC4 		
DAC_1:
#else
    cbi     _SFR_IO_ADDR(PORTB), PB4
#endif

    ldi     r20, 8
.L4:
    sbrc    r25, 7
    sbi     _SFR_IO_ADDR(PORTB), PB1
    sbrs    r25, 7
    cbi     _SFR_IO_ADDR(PORTB), PB1
    sbi     _SFR_IO_ADDR(PORTB), PB0
    lsl     r25
    cbi     _SFR_IO_ADDR(PORTB), PB0
    dec     r20
    brne    .L4

    mov     r25, r24
    ldi     r20, 8
.L5:
    sbrc    r25, 7
    sbi     _SFR_IO_ADDR(PORTB), PB1
    sbrs    r25, 7
    cbi     _SFR_IO_ADDR(PORTB), PB1
    sbi     _SFR_IO_ADDR(PORTB), PB0
    lsl     r25
    cbi     _SFR_IO_ADDR(PORTB), PB0
    dec     r20
    brne    .L5

#ifdef DUAL_DAC	
	tst 	r22
	breq    DAC_U2
DAC_I2:
	sbi 	_SFR_IO_ADDR(PORTC), PC5
	jmp 	DAC_2
DAC_U2:
	sbi		_SFR_IO_ADDR(PORTC), PC4 		
DAC_2:
#else
    sbi     _SFR_IO_ADDR(PORTB), PB4
#endif

    pop     r25
    pop     r20

    ret

.endfunc



.end

