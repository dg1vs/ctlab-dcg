/*
 * Copyright (c) 2007, 2008 by Hartmut Birr
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

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include "i2creg.h"

#define NDEBUG
#include "debug.h"

uint16_t GetADC(uint8_t Channel)
{
    uint8_t i;
    uint16_t Result = 0;

    ADMUX = (Channel & 0x07);

    for(i = 0; i < 4; i++)
    {
        ADCSRA = (1<<ADEN)|(1<<ADSC)|(1<<ADIF)|(1<<ADPS2)|(1<<ADPS1);
        while(!(ADCSRA & (1<<ADIF)));
        Result += ADC;
    }
    return (Result + 2) / 4;
}

#if 0
uint16_t ShiftIn1864(void)
{
    union
    {
        uint8_t u8[2];
        uint16_t u16;
    } data;
    uint8_t i;

    PORTB |= (1<<PB0);                  // SCLK high

    data.u16 = 0;
    _delay_us(4);

    PORTB &= ~(1<<PB7);                 // STRADC low

    _delay_us(1);

    for (i = 0; i < 8; i++)
    {
        PORTB &= ~(1<<PB0);             // SCLK low
        data.u8[1] <<= 1;
        PORTB |= (1<<PB0);              // SCLK high

        if (PINB & (1<<PB6))
        {
            data.u8[1] |= 0x01;
        }
    }

    for (i = 0; i < 8; i++)
    {
        PORTB &= ~(1<<PB0);             // SCLK low
        data.u8[0] <<= 1;
        PORTB |= (1<<PB0);              // SCLK high

        if (PINB & (1<<PB6))
        {
            data.u8[0] |= 0x01;
        }
    }


    PORTB |= (1<<PB7);                  // STRADC high

    return data.u16;
}
#endif

#if 0
void ShiftOut1655(uint16_t Value)
{
    uint8_t tmp;
    uint8_t i;

    PORTB &= ~(1<<PB0);                 // SCLK low
    PORTB &= ~(1<<PB4);                 // STRDC low

    tmp = ((uint8_t*)Value)[1];

    for (i = 0; i < 8; i++)
    {
        if (tmp & 0x80)
        {
            PORTB |= (1<<PB1);          // SDATA high
        }
        else
        {
            PORTB &= ~(1<<PB1);         // SDATA low
        }
        PORTB |= (1<<PB0);              // SCLK high
        tmp <<= 1;
        PORTB &= ~(1<<PB0);             // SCLK low
    }

    tmp = (uint8_t)Value;

    for (i = 0; i < 8; i++)
    {
        if (tmp & 0x80)
        {
            PORTB |= (1<<PB1);          // SDATA high
        }
        else
        {
            PORTB &= ~(1<<PB1);         // SDATA low
        }
        PORTB |= (1<<PB0);              // SCLK high
        tmp <<= 1;
        PORTB &= ~(1<<PB0);             // SCLK low
    }
    PORTB |= (1<<PB4);                  // STRDC high
}
#endif



void ShiftOut1257(uint16_t Value)


{
    uint8_t tmp;
    uint8_t i;

    PORTB |= (1<<PB0);                  // SCLK high
    tmp = Value >> 8;                   // delay
    PORTB |= (1<<PB4);                  // STRDC high



    for (i = 0; i < 4; i++)
    {
        PORTB &= ~(1<<PB0);             // SCLK low
        if (tmp & 0x08)
        {
            PORTB |= (1<<PB1);          // SDATA high
        }
        else
        {
            PORTB &= ~(1<<PB1);         // SDATA low
        }
        PORTB |= (1<<PB0);              // SCLK high
        tmp <<= 1;
    }

    tmp = Value;
    for (i = 0; i < 8; i++)
    {
        PORTB &= ~(1<<PB0);             // SCLK low
        if (tmp & 0x80)
        {
            PORTB |= (1<<PB1);          // SDATA high
        }
        else
        {
            PORTB &= ~(1<<PB1);         // SDATA low
        }
        PORTB |= (1<<PB0);              // SCLK high
        tmp <<= 1;
    }

    PORTB &= ~(1<<PB4);                 // STRDC low
    tmp <<= 1;                          // Dummy als Delay
    PORTB &= ~(1<<PB0);                 // SCLK low
    tmp <<= 1;                          // Dummy als Delay
    PORTB |= (1<<PB4);                  // STRDC high
}

void ConfigLM75(double Temp)
{
    uint8_t data[2];
    uint16_t Temp2;

    Temp2 = 2 * Temp;

    // Optionsregister, invertierter Ausgang, fault queue auf max
    data[0] = 0x1c;
    if (i2c_write_regs(0x90, 1, 1, data))
    {
        DPRINT(PSTR("%s: Couldn't detect a device at address 0x90\n"), __FUNCTION__);
    }


    // Tos Register,
    data[0] = Temp2 >> 1;
    data[1] = Temp2 << 7;
    i2c_write_regs(0x90, 3, 2, data);

    // Thyst. Temperatur - 5°C
    Temp2 -= 10;
    data[0] = Temp2 >> 1;
    data[1] = Temp2 << 7;
    i2c_write_regs(0x90, 2, 2, data);
}

double GetLM75Temp(void)
{
    uint8_t data[2] = {0x19, 0x00};     // 25°C
    int16_t tmp;

    i2c_read_regs(0x90, 0, 2, data);
    tmp = (data[0] << 1) | (data[1] >> 7);
    if (tmp & 0x100)
    {
        // temperature is negative, adjust the upper bits
        tmp |= 0xfe00;
    }
    return tmp / 2.0;
}

