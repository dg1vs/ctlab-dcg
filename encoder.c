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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 */

#include "config.h"

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "dcg.h"

/*
#include <string.h>
#include <stdio.h>
*/
#ifndef ENCODER_PORT
#define ENCODER_PORT    PINA
#endif

#ifndef ENCODER_PINS
#define ENCODER_PINS    (1<<PA1) | (1<<PA0)
#endif


int8_t EncPos;
uint8_t EncState = 3;



// Encoder with 2 pulses per step

// 3 - 1 - 0  and 0 - 2 - 3
// 0 and 3 are the stable states (0 = both switches closed, 3 = both switches open)
// the encoder position is incremented with the transition via 3 -> 1 -> 0 and 0 -> 2 -> 3
// the encoder position is decremented with the transition via 0 -> 1 -> 3 and 3 -> 2 -> 0
// always after completing the last transition

// encoder state 1 is called state 4 if bound for counterclockwise turn, via 0 -> 1 -> 3
// encoder state 2 is called state 5 if bound for counterclockwise turn, via 3 -> 2 -> 0


//   stb       +1  stb        +1
//
//    3 <--> 1 ---> 0 <--> 2 ---> 3
//    |             |             |
//    | <--- 4 <--> | <--- 5 <--> |
//
//       -1            -1


const PROGMEM uint8_t ucEncoderTable2[24]=
{
    0x00, 0x04, 0x02, 0x4F,     // state 0
    0x10, 0x01, 0x4F, 0x03,     // state 1 counterclockwise
    0x00, 0x4F, 0x02, 0x13,     // state 2 counterclockwise
    0x4F, 0x01, 0x05, 0x03,     // state 3
    0x00, 0x04, 0x4F, 0x23,     // state 4 (=1) clockwise
    0x20, 0x4F, 0x05, 0x03      // state 5 (=2) clockwise
};



// Encoder with 4 pulses per step

// 3 - 1 - 0 - 2 - 3
// 3 is the stable state (both swiches open)
// the encoder position is incremented with the transition via 1 -> 0 -> 2
// the encoder position is decremented with the transition via 2 -> 0 -> 1
// always after completing the last transition

// encoder state 0 is called state 4 if bound for counterclockwise turn, 2 -> 0 -> 1


//   stb               +1
//
//    3 <--> 1 <--> 0 ---> 2 <--> 3
//           |             |
//           | <--- 4 <--> |
//
//              -1


const PROGMEM uint8_t ucEncoderTable4[24] =
{
    0x00, 0x01, 0x12, 0x4F,     // state 0 clockwise
    0x00, 0x01, 0x4F, 0x03,     // state 1
    0x04, 0x4F, 0x02, 0x03,     // state 2
    0x4F, 0x01, 0x02, 0x03,     // state 3
    0x04, 0x21, 0x02, 0x4F,     // state 4 (=0) counterclockwise
    0x4F, 0x4F, 0x4F, 0x4F      // dummy state to get 24 entries.
};


// explanation for ucEncoderTableX[]:

//  entries in table define responses to a) current state of statemachine b) new encoder position
//  every line is for one state (indicated in the comment) and for new encoder pos 00, 01, 02 and 03 respectively
//  each entry has two parts:
//      high nibble : 0: no output, no error, just fine
//                    1: clockwise turn detected after way through statemachine
//                    2: counterclockwise turn detected after way through statemachine
//                    4: invalid new position, meaning a jump from 0 to 3, 3 to 0, 2 to 4, etc.
//      low nibble :  gives new state in the statemachine
//                    this is necessary since the pre-condition for a clockwise or counterclockwise turn has to be memorized
//                    and is different for clockwise and counterclockwise turn. Help to add hysteresis to the encoder.

//  every EncoderTable must contain the same number of states ( * input options) = 24, to avoid an out of bounds access
//  to a table in case the Encoder step size is switched by the user during runtime.

void Encoder_MainFunction(void)
{
    static uint8_t EncState = 3;
    uint8_t next = 0xFF;
    uint8_t i, ucPort;
    uint8_t cnt, ucValid = 0;
    uint8_t ucNewState, ucTableOffset;


    // sample the port pins for max. 5 times, record number of stable samples (first loop is to load the current value)
    for (i = cnt = 0; i < 6; i++)
    {
        _delay_us(2);
        ucPort = ENCODER_PORT & (ENCODER_PINS);

        if ( ucPort == next )
        {
            cnt++;

            // quit loop if 3 successive identical samples have been met
            if (cnt >= 3)
            {
                ucValid = 1;
                break;
            }
        }
        else
        {
            cnt = 1;
            next = ucPort;
        }
    }


    // if new state of encoder is valid go through table, even if it has not changed
    if (ucValid == 1)
    {
        ucTableOffset = (EncState<<2) + next; // calculate index in table = state * 4 + new encoder state

        switch(Params.IncRast)
        {
            default:    // well, actually no code here. To be implemented in case of new types of encoders having pulse counts other than 4 or 2
                // if no predefined code is set in eeprom, revert to "4"
            case 4:     // Encoder with 4 pulses per step
                ucNewState = pgm_read_byte(&ucEncoderTable4[ucTableOffset]);
                break;
	        case 2:     // Encoder with 2 pulses per step
                ucNewState = pgm_read_byte(&ucEncoderTable2[ucTableOffset]);
                break;
        }

        EncState = ucNewState & 0x0F;   // set next state from output of statemachine, changed later on only for "invalid"

        switch (ucNewState & 0xF0 )
        {
            case 0x00:  // no increment, or decrement, just go on
                break;
            case 0x10:  // clockwise
                EncPos++;
                break;
            case 0x20:  // counterclockwise
                EncPos--;
                break;
            case 0x40:  // invalid next step, encoder likely to be turned faster than software can follow.
                EncState = next;
                break;
        }
    }
}


int8_t Encoder_GetPosition(uint8_t reset)
{
    uint8_t sreg;
    int8_t pos;

    sreg = SREG;
    cli();

    pos = EncPos;
    if (reset)
    {
        EncPos = 0;
    }
    SREG = sreg;

    return pos;
}

int8_t Encoder_GetAndResetPosition(void)
{
    return Encoder_GetPosition(1);
}
