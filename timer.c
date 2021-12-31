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

#include "config.h"

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "timer.h"
#include "dcg.h"
#include "Encoder.h"
#include "config.h"

static volatile struct
{
    uint8_t Timer4msOV      : 1;
    uint8_t Timer10msOV     : 1;
    uint8_t Timer50msOV     : 1;
    uint8_t Timer100msOV    : 1;
    uint8_t TimersActive    : 1;
} TimerFlags;

static volatile uint32_t Ticker;
static uint8_t TimerState;

static uint8_t TimeBase4ms;
static uint8_t TimeBase10ms;
static uint8_t TimeBase100ms;

void Timer_Init(void)
{
    uint8_t sreg = SREG;
    cli();

    // initialize Timer 0, period 100us, prescaler 8, CTC mode, up to 20MHz
#if defined(__AVR_ATmega32__)
    OCR0 = (F_CPU / 80000UL) - 1;
    TCNT0 = 0;
    TCCR0 = (1<<WGM01)|(1<<CS01);
    TIMSK = (1<<OCIE0);
#elif defined(__AVR_ATmega324P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)
    OCR0A = (F_CPU / 80000UL) - 1;
    OCR0B = 0;
    TCNT0 = 0;
    TCCR0A = (1<<WGM01);;
    TCCR0B = (1<<CS01);
    TIMSK0 = (1<<OCIE0A);
#else
#error Please define your TIMER0 code
#endif

    // Timer 2
#if defined(__AVR_ATmega32__)
#if (F_CPU > 16000000UL)
    // initialize Timer 2, period 500us, prescaler 64, CTC mode, more than 16MHz
    OCR2 = (F_CPU / 128000UL) - 1;
    TCNT2 = 0;
    TCCR2 = (1<<WGM21)|(1<<CS22);
    TIMSK |= (1<<OCF2);
#else
    // initialize Timer 2, period 500us, prescaler 32, CTC mode, up to 16 MHz
    OCR2 = (F_CPU / 64000UL) - 1;
    TCNT2 = 0;
    TCCR2 = (1<<WGM21)|(1<<CS21)|(1<<CS20);
    TIMSK |= (1<<OCF2);
#endif
#elif defined(__AVR_ATmega324P__) || defined(__AVR_ATmega644__)  || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)
#if (F_CPU > 16000000UL)
    // initialize Timer 2, period 500us, prescaler 64, CTC mode, more than 16MHz
    OCR2A = (F_CPU / 128000UL) - 1;
    OCR2B = 0;
    TCNT2 = 0;
    TCCR2A = (1<<WGM21);
    TCCR2B = (1<<CS22);
    TIMSK2 |= (1 << OCIE2A);
#else
    // initialize Timer 2, period 500us, prescaler 32, CTC mode, up to 16 MHz
    OCR2A = (F_CPU / 64000UL) - 1;
    OCR2B = 0;
    TCNT2 = 0;
    TCCR2A = (1<<WGM21);
    TCCR2B = (1<<CS21)|(1<<CS20);
    TIMSK2 |= (1<<OCIE2A);
#endif
#else
#error Please define your TIMER2 code
#endif


    TimerFlags.TimersActive = 0;

    SREG = sreg;
}




#if defined(__AVR_ATmega32__)
ISR(TIMER0_COMP_vect)
#elif defined(__AVR_ATmega324P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)
ISR(TIMER0_COMPA_vect)
#else
#error Please define your TIMER0 code
#endif
{
    Ticker++;
}





#if defined(__AVR_ATmega32__)
ISR(TIMER2_COMP_vect)
#elif defined(__AVR_ATmega324P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)
ISR(TIMER2_COMPA_vect)
#else
#error Please define your TIMER2 code
#endif
{
    static uint8_t lastRangeU = 0xff;
    static uint8_t stateRangeU = 0xff;
    static uint8_t lastRangeI = 0xff;
    static uint8_t stateRangeI = 0xff;
#ifdef DUAL_DAC
    static uint16_t lastUDACOut = 0xffff;
    static uint16_t lastIDACOut = 0xffff;
#endif
    uint8_t i;

    typedef union
    {
        uint32_t u32;
        uint16_t u16;
    } Values;

#ifdef DUAL_DAC
    typedef struct
    {
        uint16_t U;
        uint16_t I;
    } DACData;

    static DACData DACOut =
    {
        .U = 0,
        .I = 0,
    };
#endif

    static Values Value;

// Ripple Values (internal to ISR)
    static uint8_t TimeRippleLow = 0;
    static uint8_t RippleModeOn = 0;
    static int16_t RippleTicker = 0;

    static uint8_t  RippleLowToHighU = 0;
    static uint8_t  RippleHighToLowU = 0;

    static uint8_t  RippleLowToHighI = 0;
    static uint8_t  RippleHighToLowI = 0;


// Arbitrary Values (internal to ISR)
    static uint8_t ArbIndex = 0; // Index in the Array
    static uint16_t ArbTmr = 0; // Timer for Ticks
    static uint16_t ArbDlyTmr = 0; // Delay Timer for Ticks

// for debugging standard hardware on Dual-DAC hardware
#ifdef DEBUGSTDHW
    static uint8_t ArbToggle=0;
#endif

    // Interrupts freigeben
#if defined(__AVR_ATmega32__)
    TIMSK &= ~(1<<OCF2);
#elif defined(__AVR_ATmega324P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)
    TIMSK2 &= ~(1<<OCIE2A);
#else
#error Please define your TIMER2 code
#endif
    sei();


    // check Ripple Mode, switch on/off synchronized with DACMux

#ifdef DUAL_DAC
    // for DUAL_DAC steps in 1ms are allowed
    if (TimerState & 0x01)
#else
    // for standard DCG hardware only steps in 2ms are allowed
    if (TimerState == 3)
#endif
    {
        if ((RippleModeOn == 0) && (TmrRippleMod != 0) && (TmrRippleOn > 0) && (TmrRippleOff > 0))
        {
            //init ripple mode and switch it on
            RippleModeOn = 1;
            RippleTicker = TmrRippleOn;
            TimeRippleLow = 0;
        }

        if ((RippleModeOn == 1) && ((TmrRippleMod == 0) || (TmrRippleOn == 0) || (TmrRippleOff == 0) ))
        {
            // switch ripple mode off
            RippleModeOn = 0;
            RippleTicker = 0;
            TimeRippleLow = 0;
        }

        if (RippleModeOn == 1)
        {
            // check current RippleTime, toggle OutputVoltage, if necessary
            if (RippleTicker <= 0)      // if countdown is zero, toggle to...
            {
                if (TimeRippleLow == 0)
                {
                    TimeRippleLow = 1;  // ripple low output voltage
                    RippleTicker = TmrRippleOff;
                    RippleHighToLowU = 1;
                    RippleHighToLowI = 1;
                }
                else
                {
                    TimeRippleLow = 0;  // default High output voltage
                    RippleTicker = TmrRippleOn;
                    RippleLowToHighU = 1;
                    RippleLowToHighI = 1;
                }
            }

#ifdef DUAL_DAC
            // for DUAL_DAC steps in 1ms are allowed
            RippleTicker--;
#else
            // for standard DCG hardware only steps in 2ms are allowed
            RippleTicker -= 2;
#endif
        }
    }


    if (TimerState & 0x01)
    {
#ifndef DUAL_DAC

        // MUX für DAC abschalten
        PORTC &= ~(1<<PC4);     // MPXU abschalten, 0=off
        PORTC |= (1<<PC5);      // MPXI abschalten, 1=off


        if (TimerState & 0x02)
        {
#endif
//*** Switch U Range, if necessary *************************************************
            // 3 -> Spannung ausgeben
            if (lastRangeU != RangeU)
            {
                // Spannungsbereich hat sich geändert
                stateRangeU = 0;
                lastRangeU = RangeU;
            }
            switch (stateRangeU)
            {
                default:
#ifdef DUAL_DAC
                    if ( ArbActive == 0 )
                    {
                        if ((RippleModeOn == 1) && (TimeRippleLow == 1))
                        {
                            DACOut.U = DACRawURipple;
                        }
                        else
                        {
                            DACOut.U = DACRawU;
                        }
                    }
                    else
//*** Code for Arbitrary Mode for DualDAC mode ****************************************************
#ifndef DEBUGSTDHW
// normal code for DualDAC
                    {
                        if ( (ArbT[ArbIndex] == 0 ) || (ArbTrigger == 0x00))        // at the end of a complete sequence OR if repetitions are over
                        {
                            DACOut.U = ArbDAC[ArbIndex];
                            ArbTmr   = 0;
                            ArbIndex = 0;
                            ArbDlyTmr = ArbDelayISR;
                            if ( (ArbTrigger != 0xff) && (ArbTrigger != 0x00) )
                            {
                                ArbTrigger--;
                            }
                        }
                        else if (ArbDlyTmr == 0)
                        {
                            if ( ArbTmr < ArbT[ArbIndex] ) // to avoid ArbTmr out of range in case of change of sequence

                            {
                                DACOut.U = (int32_t)ArbDAC[ArbIndex] + ((int32_t)ArbDAC[ArbIndex+1] - (int32_t)ArbDAC[ArbIndex]) * (int32_t)ArbTmr / (int32_t)ArbT[ArbIndex];
                                ArbTmr++;
                            }
                        }
                        else
                        {
                            DACOut.U = ArbDAC[ArbIndex];
                            ArbDlyTmr--;
                        }

                        if ( ArbTmr >= ArbT[ArbIndex] )                             // at the end of the time between two steps
                        {
                            ArbTmr   = 0;
                            ArbIndex++;

                            if ( ArbT[ArbIndex] == 0 )
                            {
                                ArbIndex = 0;
                                ArbDlyTmr = ArbDelayISR;
                                if ( (ArbTrigger != 0xff) && (ArbTrigger != 0x00) )
                                {
                                    ArbTrigger--;
                                }
                            }
                        }
                    }

//****************************
#else // !DEBUGSTDHW
                    {
                        if (ArbToggle == 1)
                        {
                            ArbToggle = 0;
                        }
                        else
                        {
                            ArbToggle =1;

//****************************
//begin of copy from standard routine
//replace Value.u16 by DACOut.U
                            {
                                if (( ArbT[ArbIndex] == 0 ) || (ArbTrigger == 0x00))
                                {
                                    DACOut.U = ArbDAC[ArbIndex];
                                    ArbTmr   = 0;
                                    ArbIndex = 0;
                                    ArbDlyTmr = ArbDelayISR;
                                    if ( (ArbTrigger != 0xff) && (ArbTrigger != 0x00) )
                                    {
                                        ArbTrigger--;
                                    }
                                }
                                else if (ArbDlyTmr == 0)
                                {
                                    if ( ArbTmr < ArbT[ArbIndex] ) // to avoid ArbTmr out of range in case of change of sequence
                                    {
                                        DACOut.U = (int32_t)ArbDAC[ArbIndex] + ((int32_t)ArbDAC[ArbIndex+1] - (int32_t)ArbDAC[ArbIndex]) * (int32_t)ArbTmr / (int32_t)ArbT[ArbIndex];
                                        ArbTmr+=2;      // timer increment is 2 for standard hardware
                                    }
                                }
                                else
                                {
                                    DACOut.U = ArbDAC[ArbIndex];

                                    if (ArbDlyTmr == 1) // just in case some uneven value made its way to this point
                                    {
                                        ArbDlyTmr = 0;
                                    }
                                    else
                                    {
                                        ArbDlyTmr-=2;
                                    }
                                }


                                while ( ArbTmr >= ArbT[ArbIndex] )
                                {
                                    ArbTmr -= ArbT[ArbIndex];       // not set to zero to handle samples with uneven ms
                                    ArbIndex++;

                                    if ( ArbT[ArbIndex] == 0 )
                                    {
                                        ArbIndex = 0;
                                        ArbTmr   = 0;           // at the end of a sequence, reset ArbTmr to "zero", even if it was "one". This can change the sequence by 1 ms (in case of standard hardware)
                                        // This is to:
                                        // - re-synchronize ArbTmr after switching of sequences (to avoid starting sequence with ArbTmr=1)
                                        // - avoid toggling waveform between two sample sets (starting with alternating ArbTmr=0 or =1), if the total time of the sequence is odd ms.
                                        ArbDlyTmr = ArbDelayISR;
                                        if ( (ArbTrigger != 0xff) && (ArbTrigger != 0x00) )
                                        {
                                            ArbTrigger--;
                                        }
                                        break;                  // to avoid endless loop if the first sample is time accidentially zero
                                    }
                                }


                            }
//end of copy from standard routine
//****************************
                        }
                    }
#endif // DEBUGSTDHW
                    if (Params.OutputOnOff == 0)
                        DACOut.U = 0;

//*** Code for Arbitrary Mode for single lDAC mode  *************************************************
#else
                    if ( ArbActive == 0 )
                    {
                        if ((RippleModeOn == 1) && (TimeRippleLow == 1))
                        {
                            Value.u16 = DACRawURipple;
                        }
                        else
                        {
                            Value.u16 = DACRawU;
                        }
                    }
                    else
//*** Code for Arbitrary Mode for standard DAC mode *************************************************
                    {
                        if (( ArbT[ArbIndex] == 0 ) || (ArbTrigger == 0x00))
                        {
                            Value.u16 = ArbDAC[ArbIndex];
                            ArbTmr   = 0;
                            ArbIndex = 0;
                            ArbDlyTmr = ArbDelayISR;
                            if ( (ArbTrigger != 0xff) && (ArbTrigger != 0x00) )
                            {
                                ArbTrigger--;
                            }
                        }
                        else if (ArbDlyTmr == 0)
                        {
                            if ( ArbTmr < ArbT[ArbIndex] ) // to avoid ArbTmr out of range in case of change of sequence
                            {
                                Value.u16 = (int32_t)ArbDAC[ArbIndex] + ((int32_t)ArbDAC[ArbIndex+1] - (int32_t)ArbDAC[ArbIndex]) * (int32_t)ArbTmr / (int32_t)ArbT[ArbIndex];
                                ArbTmr+=2;      // timer increment is 2 for standard hardware
                            }
                        }
                        else
                        {
                            Value.u16 = ArbDAC[ArbIndex];

                            if (ArbDlyTmr == 1) // just in case some uneven value made its way to this point
                            {
                                ArbDlyTmr = 0;
                            }
                            else
                            {
                                ArbDlyTmr-=2;
                            }
                        }


                        while ( ArbTmr >= ArbT[ArbIndex] )
                        {
                            ArbTmr -= ArbT[ArbIndex];       // not set to zero to handle samples with uneven ms
                            ArbIndex++;

                            if ( ArbT[ArbIndex] == 0 )
                            {
                                ArbIndex = 0;
                                ArbTmr   = 0;           // at the end of a sequence, reset ArbTmr to "zero", even if it was "one". This can change the sequence by 1 ms (in case of standard hardware)
                                // This is to:
                                // - re-synchronize ArbTmr after switching of sequences (to avoid starting sequence with ArbTmr=1)
                                // - avoid toggling waveform between two sample sets (starting with alternating ArbTmr=0 or =1), if the total time of the sequence is odd ms.
                                ArbDlyTmr = ArbDelayISR;
                                if ( (ArbTrigger != 0xff) && (ArbTrigger != 0x00) )
                                {
                                    ArbTrigger--;
                                }
                                break;                  // to avoid endless loop if the first sample is time accidentially zero
                            }
                        }
                    }

                    if (Params.OutputOnOff == 0)
                        Value.u16 = 0;

//*** ******************************************** *************************************************
#endif
                    break;

                case 0:
                case 1:
                    // für 2 Zyklen (2x1ms) 0V ausgeben
#ifdef DUAL_DAC
                    DACOut.U = 0;
#else
                    Value.u16 = 0;
#endif
                    stateRangeU++;
                    break;

                case 2:
                case 3:
                    // für 2 Zyklen (2x1ms) 0V ausgeben und Spannungsbereich umschalten
#ifdef DUAL_DAC
                    DACOut.U = 0;
#else
                    Value.u16 = 0;
#endif
                    if (lastRangeU)
                    {
                        PORTB |= (1<<PB5);
                    }
                    else
                    {
                        PORTB &= ~(1<<PB5);
                    }
                    stateRangeU++;
                    break;
            }
#ifndef DUAL_DAC
        }
        else
        {
#endif
//*** Switch I Range, if necessary *************************************************
            // 1 -> Strom ausgeben
            if (lastRangeI != RangeI)
            {
                // Strombereich hat sich geändert
                stateRangeI = 0;
                lastRangeI = RangeI;
            }
            switch (stateRangeI)
            {
                default:
#ifdef DUAL_DAC
                    DACOut.I = DACRawI;
#else
                    Value.u16 = DACRawI;
#endif
                    break;

                case 0:
                case 1:
                    // für 2 Zyklen (2x1ms) 0A ausgeben
#ifdef DUAL_DAC
                    DACOut.I = 0;
#else
                    Value.u16 = 0;
#endif
                    stateRangeI++;
                    break;

                case 2:
                case 3:
                    // für 2 Zyklen (2x1ms) 0A ausgeben umd Strombereich umschalten
#ifdef DUAL_DAC
                    DACOut.I = 0;
#else
                    Value.u16 = 0;
#endif
                    PORTC = (PORTC & ~((1<<PC3)|(1<<PC2))) | (~(lastRangeI << 2) & ((1<<PC3)|(1<<PC2)));
                    stateRangeI++;
                    break;
            }
#ifndef DUAL_DAC
        }
#endif

//*** Loading DACs *************************************************

#ifdef DUAL_DAC
        if (lastUDACOut != DACOut.U)
        {
            lastUDACOut = DACOut.U;

            if (Params.Options.DAC16Present)
            {
                ShiftOut1655(DACOut.U, 0); //Lade den Spannungs-DAC
            }
            else
            {
                ShiftOut1257(DACOut.U);
            }
        }
        if (lastIDACOut != DACOut.I)
        {
            lastIDACOut = DACOut.I;

            if (Params.Options.DAC16Present)
            {
                ShiftOut1655(DACOut.I, 1); //Lade den Strom-DAC
            }
            else
            {
                ShiftOut1257(DACOut.I);
            }
        }

#else

        if (Params.Options.DAC16Present)
        {
            ShiftOut1655(Value.u16);
        }
        else
        {
            ShiftOut1257(Value.u16);
        }
#endif

        // Periodische Timer aktualisieren
        if (TimerFlags.TimersActive)
        {
            if (++TimeBase4ms >= TIMER_4MS)
            {
                TimerFlags.Timer4msOV = 1;
                TimeBase4ms = 0;
            }
            if (++TimeBase10ms >= TIMER_10MS)
            {
                TimerFlags.Timer10msOV = 1;
                TimeBase10ms = 0;
            }
            if (++TimeBase100ms >= TIMER_100MS)
            {
                TimerFlags.Timer100msOV = 1;
                TimeBase100ms = 0;
            }
            if (TimeBase100ms == 0 ||
                    TimeBase100ms == TIMER_50MS)
            {
                TimerFlags.Timer50msOV = 1;
            }
        }
        Encoder_MainFunction();                   // runtime is part of the DAC settle time
        _delay_us(20);                  // more settle time

#ifndef DUAL_DAC
        if (TimerState & 0x02)
        {
            // 3
            PORTC |= (1<<PC4);          // MUXU für DAC einschalten, 1=on
        }
        else
        {
            // 1
            PORTC &= ~(1 << PC5);       // MPXI für DAC einschalten, 0=on
        }
#endif

    }
    else
    {
        // AD Wandlung
        if (Params.Options.ADC16Present)
        {
            PORTB |= (1<<PB0);          // SCLK high
            _delay_us(1);
            PORTB &= ~(1<<PB7);         // STRADC low
            Value.u32 = 0;
            _delay_us(4);
            PORTB |= (1<<PB7);          // STRADC high, Wandlung wird gestartet
            ShiftIn1864();
            for (i = 0; i < 4; i++)
            {
                Value.u32 += ShiftIn1864();
            }
            Value.u32 /= 4;

            if (TimerState & 0x02)
            {
                // 2, get voltage value every 2ms, (interlaced with current value 1ms later)

#ifdef DUAL_DAC
#define SETTLETIME 15 // 10     // in DUAL DAC mode settling time is faster... but the main reason is slow RC combination C11
#else
#define SETTLETIME 25 // 20
#endif

                // depending on the mode,
                if ( // no ripple mode => always -or-
                    (RippleModeOn == 0) ||
                    // Arbitrary Mode on (is dominant over ripple mode)
                    (ArbActive == 1)  )
                {
                    ADCRawU = ADCRawULow = Value.u16;
                }

                if ((RippleModeOn == 1) && (ArbActive == 0))
                {
                    // high level after settling time
                    if (( (TimeRippleLow == 0) && (((TmrRippleOn - RippleTicker) >= SETTLETIME) || (RippleTicker <= 1)) ) ||
                            ((RippleLowToHighU == 1) && (TmrRippleOn < 3)) )
                    {
//PORTD |= (1<<PD7);
                        ADCRawU = Value.u16;
                        RippleLowToHighU = 0;
//PORTD &= ~(1<<PD7);
                    }
                    // low level after settling time
                    else if  (( (TimeRippleLow == 1) && (((TmrRippleOff - RippleTicker) >= SETTLETIME) || (RippleTicker <= 1)) ) ||
                              ((RippleHighToLowU == 1) && (TmrRippleOff < 3)) )
                    {
//PORTD |= (1<<PD7);
                        ADCRawULow = Value.u16;
                        RippleHighToLowU = 0;
//PORTD &= ~(1<<PD7);
                    }
                }

                PORTC &= ~(1<<PC6);     // MUX für ADC auf I
            }
            else
            {
                // 0, get current value

                // depending on the mode,
                if ( // no ripple mode => always -or-
                    (RippleModeOn == 0) ||
                    // Arbitrary Mode on (is dominant over ripple mode)
                    (ArbActive == 1)  )
                {
                    ADCRawI = ADCRawILow = Value.u16;
                }

                if ((RippleModeOn == 1) && (ArbActive == 0))
                {
                    // high level after settling time
                    if (( (TimeRippleLow == 0) && (((TmrRippleOn - RippleTicker) >= SETTLETIME) || (RippleTicker <= 1)) ) ||
                            ((RippleLowToHighI == 1) && (TmrRippleOn < 3)) )
                    {
//PORTD |= (1<<PD7);
                        ADCRawI = Value.u16;
                        RippleLowToHighI = 0;
//PORTD &= ~(1<<PD7);
                    }
                    // low level after settling time
                    else if  (( (TimeRippleLow == 1) && (((TmrRippleOff - RippleTicker) >= SETTLETIME) || (RippleTicker <= 1)) ) ||
                              ((RippleHighToLowI == 1) && (TmrRippleOff < 3)) )
                    {
//PORTD |= (1<<PD7);
                        ADCRawILow = Value.u16;
                        RippleHighToLowI = 0;
//PORTD &= ~(1<<PD7);
                    }

                }

                //              ADCRawI = Value.u16;


                PORTC |= (1<<PC6);      // MUX für ADC auf U
            }
        }
        Encoder_MainFunction();
    }



    if (++TimerState >= 4)
    {
        TimerState = 0;
    }

    // Interrupts sperren
    cli();
#if defined(__AVR_ATmega32__)
    TIMSK |= (1<<OCF2);
#elif defined(__AVR_ATmega324P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)
    TIMSK2 |= (1<<OCIE2A);
#else
#error Please define your TIMER2 code
#endif
}

void Timer_StartTimers(void)
{
    uint8_t sreg;

    sreg = SREG;
    cli();

    TimerFlags.Timer4msOV = 0;
    TimerFlags.Timer10msOV = 0;
    TimerFlags.Timer50msOV = 0;
    TimerFlags.Timer100msOV = 0;
    TimerFlags.TimersActive = 1;

    TimeBase4ms = 0;
    TimeBase10ms = 0;
    TimeBase100ms = 0;

    SREG = sreg;
}

uint8_t Timer_TestAndResetTimerOV(uint8_t TimerId)
{
    uint8_t result = 0;
    uint8_t sreg;

    switch(TimerId)
    {
        case TIMER_4MS:
            sreg = SREG;
            cli();
            if (TimerFlags.Timer4msOV)
            {
                TimerFlags.Timer4msOV = 0;
                result = 1;
            }
            SREG = sreg;
            break;

        case TIMER_10MS:
            sreg = SREG;
            cli();
            if (TimerFlags.Timer10msOV)
            {
                TimerFlags.Timer10msOV = 0;
                result = 1;
            }
            SREG = sreg;
            break;

        case TIMER_50MS:
            sreg = SREG;
            cli();
            if (TimerFlags.Timer50msOV)
            {
                TimerFlags.Timer50msOV = 0;
                result = 1;
            }
            SREG = sreg;
            break;

        case TIMER_100MS:
            sreg = SREG;
            cli();
            if (TimerFlags.Timer100msOV)
            {
                TimerFlags.Timer100msOV = 0;
                result = 1;
            }
            SREG = sreg;
            break;
    }
    return result;
}

uint32_t Timer_GetTicker(void)
{
    uint32_t result;
    uint8_t sreg;

    sreg = SREG;
    cli();

    result = Ticker;

    SREG = sreg;

    return result;
}

void Timer_Wait_us(uint32_t us)
{
    uint32_t start, stop, current;

    current = start = Timer_GetTicker();

    stop = start + us / 100;

    if (stop < start)
    {
        while (current >= start)
        {
            current = Timer_GetTicker();
        }
    }
    while (current < stop)
    {
        current = Timer_GetTicker();
    }
}
