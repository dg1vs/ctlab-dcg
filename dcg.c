/*
 * Copyright (c) 2009, 2010, 2012, 2014 by Paul Schmid
 * Copyright (c) 2007, 2008 by Hartmut Birr
 *
 * "basierend auf c't-Lab von Carsten Meyer, c't magazin"
 *
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

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include <string.h>

#include "config.h"
#include "Uart.h"
#include "Parser.h"
#include "timer.h"
#include "i2creg.h"
#include "Lcd.h"
#include "encoder.h"
#include "dcg.h"

#define NDEBUG
#include "debug.h"


#ifdef DEFAULT_PARAMS_16BIT

PARAMS Params =
{
    .DACUOffsets        = {10, 10},                         // 0.38mV
    .DACUScales         = {1.0, 1.0},
    .DACIOffsets        = {40, 40, 40, 40},                 // 1.5mV
    .DACIScales         = {1.0, 1.0, 1.0, 1.0},
    .ADCUOffsets        = {-306, -306},                     // -R15/(R14+R15) * 2^16
    .ADCUScales         = {1.0047, 1.0047},                 // (R15+R14)/R14
    .ADCIOffsets        = {-306, -306, -306, -306},         // -R15/(R14+R15) * 2^16
    .ADCIScales         = {1.0047, 1.0047, 1.0047, 1.0047}, // (R15+R14)/R14
    .InitVoltage        = 5.0,                              // 5V
    .InitCurrent        = 0.02,                             // 20mA
    .GainPre            = 3.0,                              // Gain OP-Amp. after ADC, (R17/R18)+1
    .GainOut            = 3.0,                              // Gain Sense-Diff.-Amp., R21/R20
    .GainI              = 0.25,                             // R33/(R33+R34)
    .RefVoltage         = 2.5,
    .MaxVoltage         = {12.1, 30.0},                     // Max. voltage of both U ranges
    .RSense             = {470.0, 47.0, 4.7, 0.47},         // Shunt values of the four current ranges
    .MaxCurrent         = {0.002, 0.02, 0.2, 2.0},          // Max. current of the four current ranges
    .ADCUfacs           = {2, 6},                           // (R12+R24)/R12 and (R24+(R12||R16))/(R12||R16)
    .Options            = {
                              .DAC16Present = 1,
                              .ADC16Present = 1,
                              .SuppressFuseblowMsg = 0
                      },
    .RelayVoltage       = 12.5,
    .FanOnTemp          = 50,
    .RippleOn           = 0,                                // Ripple High time (ms)
    .RippleOff          = 0,                                // Ripple Low time (ms)
    .RippleMod          = 0,                                // Ripple Percentage
    .IncRast            = 4,
    .SerBaudReg         = INIT_UBRR % 256,
    .TrackChSave        = 255,
    .GainPwrIn          = 19,
    .LockRangeI         = 255,
    .LockRangeU         = 255,
    .Initialised        = 0xaa55,
    .OutputOnOff        = 1,
};

#endif


#ifdef DEFAULT_PARAMS_12BIT

PARAMS Params =
{
    .DACUOffsets        = {5, 5},                           // 2.5mV
    .DACUScales         = {1.0, 1.0},
    .DACIOffsets        = {10, 10, 10, 10},                 // 5mV
    .DACIScales         = {1.0, 1.0, 1.0, 1.0},
    .ADCUOffsets        = {0, 0},
    .ADCUScales         = {1.0, 1.0},
    .ADCIOffsets        = {0, 0, 0, 0},
    .ADCIScales         = {1.0, 1.0, 1.0, 1.0},
    .InitVoltage        = 5.0,                              // 5V
    .InitCurrent        = 0.02,                             // 20mA
    .GainPre            = 5.0,                              // Gain OP-Amp. after ADC, (R17/R18)+1
    .GainOut            = 3.0,                              // Gain Sense-Diff.-Amp., R21/R20
    .GainI              = 0.5,                              // R33/(R33+R34)
    .RefVoltage         = 2.048,
    .MaxVoltage         = {6.0, 20.0},                      // Max. voltage of both U ranges
    .RSense             = {470.0, 47.0, 4.7, 0.47},         // Shunt values of the four current ranges
    .MaxCurrent         = {0.002, 0.02, 0.2, 2.0},          // Max. current of the four current ranges
    .ADCUfacs           = {1, 5},                           // (R12+R24)/R12 and (R24+(R12||R16))/(R12||R16)
    .Options            = {
                              .DAC16Present = 0,
                              .ADC16Present = 0,
                              .SuppressFuseblowMsg = 0
                          },
    .RelayVoltage       = 12.5,
    .FanOnTemp          = 50,
    .RippleOn           = 0,                                // Ripple High time (ms)
    .RippleOff          = 0,                                // Ripple Low time (ms)
    .RippleMod          = 0,                                // Ripple Percentage
    .IncRast            = 4,
    .SerBaudReg         = INIT_UBRR % 256,
    .TrackChSave        = 255,
    .GainPwrIn          = 19,
    .LockRangeI         = 255,
    .LockRangeU         = 255,
    .Initialised        = 0xaa55,
    .OutputOnOff        = 1,
};

#endif

//*** EEPROM ************************************************************************

PARAMS eepParams EEMEM;

//*** USERSETS **********************************************************************
USERPARAMS UserEepParams[USERSETS] EEMEM;
USERPARAMS UserEepParamsTmp;

uint8_t StartParamSet EEMEM;
uint8_t wStartParamSet;

//***********************************************************************************


#ifdef DUAL_DAC
const char VersStrLong[] = VERSSTRLONG_DUAL;
#else
const char VersStrLong[] = VERSSTRLONG_SINGLE;
#endif
// moving VersStrLong to PROGMEM uses 66 bytes of code, but saves 24 bytes of RAM.

#ifdef DUAL_DAC
const PROGMEM char VersStrShort_P[]= VERSSTRSHORT_DUAL;
#else
const PROGMEM char VersStrShort_P[]= VERSSTRSHORT_SINGLE;
#endif

const PROGMEM char InitEEPStr_P[]= "Init EEP";

uint8_t SlaveCh;
uint8_t TrackCh;
uint16_t ErrCount;
uint8_t ActivityTimer;
uint8_t RangeI;
uint8_t RangeU;
uint8_t LockRangeU;
uint8_t LockRangeI;
uint8_t InitLockRangeU;
uint8_t InitLockRangeI;

STATUS Status;
FLAGS Flags;

uint16_t ADCRawU;
uint16_t ADCRawULow;
uint16_t ADCRawI;
uint16_t ADCRawILow;

uint16_t DACRawU;
uint16_t DACRawI;
uint16_t DACMax;

uint16_t DACRawURipple;

int16_t TmrRippleMod = 0;
int16_t TmrRippleOn = 0;
int16_t TmrRippleOff = 0;

double Temperature;
double DACLSBU[2];
double ADCLSBU[2];
double DACLSBI[4];
double ADCLSBI[4];
double PwrInFac;
double xVoltage;
double xVoltageLow;
double xCurrent;
double xCurrentLow;
double xPower = 0.0;
double xPowerTot = 0.0;
double xAmpHours = 0.0;
double xWattHours = 0.0;

double xMeanVoltage = 0.0;
double xMeanCurrent = 0.0;

double DCVoltMod;
double DCAmpMod;

double wVoltage;
double wCurrent;
double RippleVoltage;

uint8_t ToggleDisplay = 0x00;
uint8_t ActiveParamSet = 0;

uint16_t RippleActive = 0;


//*** Parameters for operation of arbitrary sequence mode ***************************

// variables for operation, currently not stored EEPROM
uint8_t ArbActive = 0;      // for Parameter 182
                            // 0 = off , 1 = ROM, 2 = RAM   // ArbActive must be off for boot sequence, to avoid unwanted voltages at the output
#define ARBACTIVESTART 0

uint8_t ArbSelect = 0;      // for Parameter 183
                            // select ROM predefined sequence

uint8_t ArbRepeat = 0xff;   // for Parameter 184
                            // 0 = off, 1..250: single shot .. multiple, 0xff = continuous

int16_t ArbDelay = 0;       // for Parameter 185
                            // 0 = off, 1..30000 in ms

double ArbRAMtmpV = 0.0;    // for Parameter 186 = Loading Voltage values into array one-by-one

uint16_t ArbRAMtmpT = 0;    // for Parameter 187 = Loading Time values into array one-by-one

uint8_t ArbUpdateMode = 0;  // for Parameter 188 = Selection of Arbitrary RAM mode handling
                            // 0 = normal Arb operation,
                            // 1 = Loading values (Arb disabled), by parameters 186+187, repeatedly
                            // 2 = Finalizing loading of values: filling rest of memory with default data, falling back to Mode 0
                            // 3 = Storing Arb Sequence in EEPROM, falling back to Mode 0
                            // 4 = Recalling Arb Sequence from EEPROM, falling back to Mode 0

uint8_t ArbSelectRAM = 0;   // for Parameter 189
                            // select RAM predefined sequence

uint8_t ArbRAMOffset = 0;   // Offset in RAM array, ArbRAMOffset = get_SequenceStart_RAMarray(&ArbSelectRAM); internally used only.

double ArbMinVoltage = 0.0; // to calculate the voltage range for Arbitrary Mode with respect to relay switching; internally used only

//***********************************************************************************


//*** Arbitrary Mode variables ******************************************************
#define  ARBINDEXMAX 50
uint16_t ArbDAC[ARBINDEXMAX];   // DAC in raw values
uint16_t ArbT[ARBINDEXMAX];     // Duration in ms
uint8_t  ArbTrigger = 0;        // ISR value for ArbRepeat
int16_t  ArbDelayISR = 0;       // ISR value for ArbDelay
// for interrupt routine --> timer.c
//***********************************************************************************


//*** Arbitrary sequences in ROM (!) ************************************************
#define ARBSEQUENCECOUNT 4

const PROGMEM  char ArbL_ISO4[9]      = "ISO4    ";
const PROGMEM double ArbV_ISO4[]      = {  1.0,  1.0,0.416667, 0.416667,   0.666667,  0.666667 ,1.0}; // ISO-4
const PROGMEM uint16_t ArbTd_ISO4[]   = {  200,   20,   50,   10,   100,   20,    0};                 // Duration in ms

const PROGMEM char ArbL_ISO4m[9]      = "ISO4m   ";
const PROGMEM double ArbV_ISO4m[]     = {  1.0,  0.916667 ,0.416667, 0.5,   0.666667,  0.75 ,1.0};   // ISO-4 with modified shape

const PROGMEM char ArbL_Graetz[9]     = "Graetz  ";
#if defined DUAL_DAC && ! defined DEBUGSTDHW
const PROGMEM double ArbV_Graetz[]    = { 1.0000, 0.9511, 0.8090, 0.5878, 0.3090, 0.0000, 0.3090, 0.5878, 0.8090, 0.9511, 1.0000};   // Steps in Volts // Sinus
const PROGMEM uint16_t ArbTd_Graetz[] = { 1,       1,      1,      1,      1,      1,      1,      1,      1,      1,      0    };   // Duration in ms
#else
const PROGMEM double ArbV_Graetz[]    = { 1.0000, 0.5878, 0.0000,  0.5878, 1.0000, 1.0000};   // Steps in Volts // Sinus
const PROGMEM uint16_t ArbTd_Graetz[] = { 2,       2,      2,      2,      2,       0    };   // Duration in ms
#endif


const PROGMEM char ArbL_3Peaks[9]     = "3Peaks  ";
#if defined DUAL_DAC && ! defined DEBUGSTDHW
const PROGMEM double ArbV_3Peaks[]    = { 1.0, 1.0,  0.416667,  1.0,  1.0,  0.666667 ,  1.0,  1.0,  0.833333 ,1.0}; // 3 Peaks down to 5V, 8V and 10V
const PROGMEM uint16_t ArbTd_3Peaks[] = { 200,   1,   1,         10,    1,     1,       10,    1,     1,       0};  // Duration in ms
#else
const PROGMEM double ArbV_3Peaks[]    = { 1.0, 1.0,  0.416667,  1.0,  1.0,  0.666667 , 1.0,  1.0,  0.833333 , 1.0, 1.0};  // 3 Peaks down to 5V, 8V and 10V
const PROGMEM uint16_t ArbTd_3Peaks[] = { 198,   2,   2,        8,    2,     2,       8,    2,     2,       2  , 0};    // Duration in ms
#endif

const char PROGMEM* const PROGMEM ArbArrayL[ARBSEQUENCECOUNT] =
{
    ArbL_ISO4,
    ArbL_ISO4m,
    ArbL_Graetz,
    ArbL_3Peaks
};

const double PROGMEM* const PROGMEM ArbArrayV[ARBSEQUENCECOUNT] =
{
    ArbV_ISO4,
    ArbV_ISO4m,
    ArbV_Graetz,
    ArbV_3Peaks
};

const uint16_t PROGMEM* const PROGMEM ArbArrayT[ARBSEQUENCECOUNT] =
{
    ArbTd_ISO4,
    ArbTd_ISO4,
    ArbTd_Graetz,
    ArbTd_3Peaks
};
//***********************************************************************************


//*** Arbitrary sequences in EEPROM (!) *********************************************
double   ArbV_EEP[ARBINDEXMAXRAM]  EEMEM;
uint16_t ArbT_EEP[ARBINDEXMAXRAM]  EEMEM;
//***********************************************************************************


//*** Arbitrary sequences in RAM (!) ************************************************
//#define ARBINDEXMAXRAM  ==> moved to dcg.h
double   ArbV_RAM[ARBINDEXMAXRAM];
uint16_t ArbT_RAM[ARBINDEXMAXRAM];

#ifdef PRELOAD_ARB_RAM

//*** predefined arbitrary sequences for RAM / for testing only ***
#if defined DUAL_DAC && ! defined DEBUGSTDHW
const PROGMEM double   ArbV_RAM_ROM[] = { 1.0, 1.0, 0.2, 0.4,  0.6,  0.5 , 1.0, 1.0000, 0.9511, 0.8090, 0.5878, 0.3090, 0.0000, 0.3090, 0.5878, 0.8090, 0.9511, 1.0000, 1.0,  0.916667 ,0.416667, 0.5,   0.666667,  0.75 ,1.0};   // Variant of ISO-4 + Graetz + ISO4
const PROGMEM uint16_t ArbT_RAM_ROM[] = { 200,  20,  50,  10,  100,   20,    0, 1,       1,      1,      1,      1,      1,      1,      1,      1,      1,      0,     200,    20,       50,     10,       100,     20,    0};   // Duration in ms
#else
const PROGMEM double   ArbV_RAM_ROM[] = { 1.0, 1.0, 0.2, 0.4,  0.6,  0.5 , 1.0, 1.0000, 0.5878, 0.0000,  0.5878, 1.0000, 1.0000, 1.0,  0.916667 ,0.416667, 0.5,   0.666667,  0.75 ,1.0};   // Variant of ISO-4 + Graetz + ISO4
const PROGMEM uint16_t ArbT_RAM_ROM[] = { 200,  20,  50,  10,  100,   20,    0, 2,       2,      2,      2,      2,       0,     200,    20,       50,     10,       100,     20,    0};   // Duration in ms

#endif

#endif

//*** initialisation of arbitrary sequence for RAM, in case the EEPROM has not already been initialized ***
void init_Arb_RAMarray(void)
{
    uint8_t i;

    for (i = 0; i < ARBINDEXMAXRAM; i++)
    {
        ArbV_RAM[i] = 1.0;
        ArbT_RAM[i] = 0;  // initialize time steps with "0"s, they are used to separate multiple sequences
    }

#ifdef PRELOAD_ARB_RAM
    memcpy_P(&ArbV_RAM, &ArbV_RAM_ROM, sizeof(ArbV_RAM_ROM));
    memcpy_P(&ArbT_RAM, &ArbT_RAM_ROM, sizeof(ArbT_RAM_ROM));
#endif

}

//*** automagic search for sequences in the RAM array ***
uint8_t get_SequenceStart_RAMarray(uint8_t* select)
{
    uint8_t i = 0;
    uint8_t first = 0;
    uint8_t found = 0;

    while (i < ARBINDEXMAXRAM)
    {
        if (ArbT_RAM[i] == 0)           // time value == 0 is the end marker of a sequence
        {
            i++;
            if ( i < ARBINDEXMAXRAM)
            {
                if (ArbT_RAM[i] != 0)   // check, if the next value is non-zero, it might be the beginning of a new sequence
                {
                    found++;            // record the number of found sequences.
                    first = i;          // record the index in the array.
                }
            }
        }
        // found the wanted sequence
        if (found == *select)           // don't change the selection
            return first;               // return the appropriate index in the array.
        i++;
    }
                        // in case the end of the RAM array has been found without finding the wanted sequence,
    *select = found;    // update the variable to the maximum available
    return first;       // return the appropriate index in the array
}
//***********************************************************************************

/*
void PrintIDNstring(void)
{

    printf_P(PSTR("#%d:254=%S "), SlaveCh, (char*)VersStrLong_P );
}
*/

int uart_putchar(char c, FILE* stream __attribute__((unused)))
{
    if (c == '\n')
    {
        c = '\r';
        while (1 != Uart_SetTxData((uint8_t*)&c, 1, 0));
        c = '\n';
    }
    while(1 != Uart_SetTxData((uint8_t*)&c, 1, 0));
    return 0;
}


static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);


void SaveUserParamSet(uint8_t set)
{
    if (set >= USERSETS) return; // out of range

    UserEepParamsTmp.InitVoltage = wVoltage;        //double
    UserEepParamsTmp.InitCurrent = wCurrent;        //double
    UserEepParamsTmp.RippleOn = Params.RippleOn;    //word = uint16
    UserEepParamsTmp.RippleOff = Params.RippleOff;  //word = uint16
    UserEepParamsTmp.RippleMod = Params.RippleMod;  //word = uint16

    UserEepParamsTmp.LockRangeU = LockRangeU;       //byte = uint8
    UserEepParamsTmp.LockRangeI = LockRangeI;       //byte = uint8
    UserEepParamsTmp.Initialised = 0xaa55;          //word = uint16

    eeprom_write_block(&UserEepParamsTmp, &UserEepParams[set], sizeof(UserEepParamsTmp));

}


uint8_t RecallUserParamSet(uint8_t set, uint8_t mode)
{
    if (set >= USERSETS) return 0xff;           // out of range

    eeprom_read_block(&UserEepParamsTmp, &UserEepParams[set], sizeof(UserEepParamsTmp));

    if (0xaa55 != UserEepParamsTmp.Initialised) return 0xfe;        // empty set

    if (mode == RECALL)
    {
        wVoltage    = UserEepParamsTmp.InitVoltage;     //double
        wCurrent    = UserEepParamsTmp.InitCurrent;     //double
        Params.RippleOn = UserEepParamsTmp.RippleOn;    //word = uint16
        Params.RippleOff = UserEepParamsTmp.RippleOff;  //word = uint16
        Params.RippleMod = UserEepParamsTmp.RippleMod;  //word = uint16

        LockRangeU = UserEepParamsTmp.LockRangeU;       //byte = uint8
        LockRangeI = UserEepParamsTmp.LockRangeI;       //byte = uint8
    }
    return set;
}




uint8_t RecallDefaultParamSet(void)
{

    if (0xaa55 == eeprom_read_word(&eepParams.Initialised))
    {
        eeprom_read_block(&Params, &eepParams, sizeof(Params));
        wVoltage = Params.InitVoltage;
        wCurrent = Params.InitCurrent;

        return 0;
    }

    else

        return 0xff;
//******************************  *****  current implementation leaves some room for improvement regarding codesize and readability
}


void PushParamSet(void)
{
    uint8_t PushSet[]=
    {
        0, // wVoltage (V)
        1, // wCurrent (A)
        27,// Params.RippleOn
        28,// Params.RippleOff
        29 // Params.RippleMod
    };

    uint8_t i = sizeof(PushSet);

    while (i>0)
    {
        ParseGetParam(PushSet[--i]);
    }

}

void LIMIT_DOUBLE(double *param, double min, double max)
{
    if (*param > max)
        *param = max;
    else if (*param < min)
        *param = min;
}

void LIMIT_UINT8(uint8_t *param, uint8_t min, uint8_t max)
{
    if (*param > max)
        *param = max;
    else if (*param < min)
        *param = min;
}

void LIMIT_INT16(int16_t *param, int16_t min, int16_t max)
{
    if (*param > max)
        *param = max;
    else if (*param < min)
        *param = min;
}

void LIMIT_UINT16(uint16_t *param, uint16_t min, uint16_t max)
{
    if (*param > max)
        *param = max;
    else if (*param < min)
        *param = min;
}


void CheckLimits(void)
{
    static uint8_t old_ArbSelectRAM = 0;

    DPRINT(PSTR("wVoltage=%.5f wCurrent=%.5f\n"), wVoltage, wCurrent);


    LIMIT_DOUBLE(&wVoltage, 0, (LockRangeU == 255) ? Params.MaxVoltage[1]:Params.MaxVoltage[LockRangeU]);
    LIMIT_DOUBLE(&wCurrent, 0, (LockRangeI == 255) ? Params.MaxCurrent[DC2000mA]:Params.MaxCurrent[LockRangeI]);


    if (TrackCh > 128)
    {
        TrackCh = 255;
    }
    else
    {
        LIMIT_UINT8(&TrackCh, 0, 7);
    }

    if (Params.OutputOnOff > 0)
    {
        Params.OutputOnOff = 1;
    }

    LIMIT_DOUBLE(&DCVoltMod, 0.0, 1.0);
    LIMIT_DOUBLE(&DCAmpMod, 0.0, 1.0);
    LIMIT_INT16(&Params.RippleMod, 0, 100);
    LIMIT_INT16(&Params.RippleOn, 0, 30000);
    LIMIT_INT16(&Params.RippleOff,0, 30000);
    LIMIT_UINT8(&ActiveParamSet, 0, 2*USERSETS);
    LIMIT_UINT8(&wStartParamSet, 0, USERSETS);


    // for standard hardware, only RippleTimes in multiples of 2 are allowed
#ifndef DUAL_DAC
    if ((Params.RippleOn % 2) == 1)
    {
        Params.RippleOn++;
    }

    if ((Params.RippleOff % 2) == 1)
    {
        Params.RippleOff++;
    }
#endif

    LIMIT_UINT8(&ArbSelect, 0 , ARBSEQUENCECOUNT-1);    // select ROM predefined sequence
    LIMIT_UINT8(&ArbActive, 0 , 2);                     // 0 = off , 1= ROM, 2= RAM
//  LIMIT_UINT8(&ArbRepeat, 0 , 255);                   // 0 = off , 1-254 count, 255= continuous  -> full range, test not necessary.
    LIMIT_INT16(&ArbDelay,  0, 30000);                  // 0 = off, 1..65000 in ms

    if (ArbSelectRAM != old_ArbSelectRAM)
    {
        ArbRAMOffset = get_SequenceStart_RAMarray(&ArbSelectRAM);
        old_ArbSelectRAM = ArbSelectRAM;
    }

    if ( (Params.RippleMod != 0) && (Params.RippleOn != 0) && (Params.RippleOff != 0) && !ArbActive)
    {
        RippleActive = 1;
    }
    else
    {
        RippleActive = 0;
    }
}


void CalcAmpWattHours(void)
{
    double dAmps = xCurrent;

    if ( RippleActive )
    {
        // reset AmpHours and WattHours in Ripple mode
        xAmpHours   = 0.0f;
        xWattHours  = 0.0f;
    }
    else if (dAmps > 0.00001)
    {
        // called every 100ms = 1/(3600 * 10) h
        // don't add anything for too low current values
        xAmpHours   += xCurrent / 36000;
        xWattHours  += xCurrent * xVoltage / 36000;
    }
}

void jobGetValues(void)
{
    static uint8_t lastRangeI = 0xff;
    static uint8_t lastRangeU = 0xff;
    static uint8_t waitITimer = 0;
    static uint8_t waitUTimer = 0;
    uint8_t sreg;
    union
    {
        int32_t i32;
        uint16_t u16[2];
    } Values, ValuesLow;

    if (lastRangeI != RangeI)
    {
        lastRangeI = RangeI;
        waitITimer = 10;    // 40ms
    }

    if (lastRangeU != RangeU)
    {
        lastRangeU = RangeU;
        waitUTimer = 10;    // 40ms
    }

    if (waitUTimer)
    {
        waitUTimer--;
    }
    else
    {
        Values.u16[1] = ValuesLow.u16[1] = 0;
        if (Params.Options.ADC16Present)
        {
            sreg = SREG;                    // for 16-bit mode only
            cli();
            Values.u16[0] = ADCRawU;        // regular measurement or RippleHigh voltage
            ValuesLow.u16[0] = ADCRawULow;  // RippleLow voltage
            SREG = sreg;
        }
        else
        {
            Values.u16[0] = ValuesLow.u16[0] = GetADC(2); // Ripple measurement not implemented in 12-bit mode
        }

        xVoltage = (Values.i32 + Params.ADCUOffsets[lastRangeU]) * ADCLSBU[lastRangeU];
        xVoltageLow = (ValuesLow.i32 + Params.ADCUOffsets[lastRangeU]) * ADCLSBU[lastRangeU];

        xMeanVoltage = (3 * xMeanVoltage + xVoltage) / 4;   // Note: Pascal FW uses 7/8
    }

    if (waitITimer)
    {
        waitITimer--;
    }
    else
    {
        Values.u16[1] =ValuesLow.u16[1] = 0;

        if (Params.Options.ADC16Present)
        {
            sreg = SREG;
            cli();
            Values.u16[0] = ADCRawI;
            ValuesLow.u16[0] = ADCRawILow;
            SREG = sreg;
        }
        else
        {
            Values.u16[0] = ValuesLow.u16[0] = GetADC(3);
        }
        xCurrent = (Values.i32 + Params.ADCIOffsets[lastRangeI]) * ADCLSBI[lastRangeI];
        xCurrentLow = (ValuesLow.i32 + Params.ADCIOffsets[lastRangeI]) * ADCLSBI[lastRangeI];

        xMeanCurrent = (3 * xMeanCurrent + xCurrent) / 4;   // Note: Pascal FW uses 7/8
    }

    xPower = xVoltage * xCurrent;

    if ( RippleActive )
    {
        xPowerTot = (xPower * Params.RippleOn + xVoltageLow * xCurrentLow * Params.RippleOff) / (Params.RippleOn + Params.RippleOff + 0.0001f);
    }
    else
        xPowerTot = xPower;

    LIMIT_DOUBLE(&xPowerTot, 0.0000001, 100000.0); // actually only to ensure non-negative values

}


double GetPowerIn(void)
{
    return GetADC(4) * PwrInFac;
}

void InitScales(void)
{
    double Ufac;
    uint32_t tmpDACMax, tmpADCMax;
    uint8_t i;

    if (Params.Options.DAC16Present)
    {
        Ufac = 2 * Params.RefVoltage;   // LTC1655 intern 2fache Verst‰rkung
        tmpDACMax = 0x10000;
    }
    else
    {
        Ufac = Params.RefVoltage;
        tmpDACMax = 0x1000;
    }
    if (Params.Options.ADC16Present)
    {
        tmpADCMax = 0x10000;
    }
    else
    {
        tmpADCMax = 0x400;
    }
    DACLSBU[0] = Ufac * Params.GainOut / (tmpDACMax * Params.DACUScales[0]);
    DACLSBU[1] = Ufac * Params.GainPre * Params.GainOut / (tmpDACMax * Params.DACUScales[1]);
    for (i = 0; i < 2; i++)
    {
        ADCLSBU[i] = Params.ADCUfacs[i] * Params.ADCUScales[i] * Params.RefVoltage * Params.GainOut / tmpADCMax;
    }

    Ufac *= Params.GainI;   // Spannungsteiler nach DAC-I auf 1/4 (0.25), R34, R33

    for (i = 0; i < 4; i++)
    {
	    // mA pro LSBit f¸r 4 Bereiche, Output:
	    DACLSBI[i] = Ufac / Params.RSense[i] / (tmpDACMax * Params.DACIScales[i]);
    }

    for (i = 0; i < 4; i++)
    {
	    // mA pro LSBit f¸r 4 Bereiche, Input:
	    ADCLSBI[i] = Params.ADCIScales[i] * Params.RefVoltage / (2 * Params.RSense[i]) / tmpADCMax; // X2 durch U11
    }

    PwrInFac = Params.RefVoltage * Params.GainPwrIn / 1024;
    DACMax = tmpDACMax - 1; // f¸r SetLevelDAC
    TrackCh = Params.TrackChSave;
    ConfigLM75(Params.FanOnTemp);

    InitLockRangeU = Params.LockRangeU;
    if (InitLockRangeU > 1 && InitLockRangeU != 255)
    {
        InitLockRangeU = 255;
    }
    InitLockRangeI = Params.LockRangeI;
    if (InitLockRangeI > DC2000mA && InitLockRangeI != 255)
    {
        InitLockRangeI = 255;
    }
}

uint8_t CalcRangeI(double I)
{
    uint8_t Range = DC2mA;

    while (Range < DC2000mA && I > Params.MaxCurrent[Range])
    {
        Range++;
    }

    return Range;
}

void SetLevelDAC(void)
{
    int32_t tmpDAC, rippleDAC;
    uint8_t Range;
    uint8_t sreg;
    uint8_t Index = 0;

    double tmpArbV;
    uint16_t tmpArbT;
    const double* ArbArrayV_Ptr;
    const uint16_t* ArbArrayT_Ptr;
    double RelativeMaxVoltage0;

//*** Conversion for Current *************************************************

    // Berechnung des Range-Wertes f¸r Strom
    if (LockRangeI == 255)
    {
        Range = CalcRangeI(wCurrent);
    }
    else
    {
        Range = LockRangeI;
    }

    if (RangeI != Range)
    {
        DCAmpMod = 1; // Prozent-Faktor r¸cksetzen
        DPRINT(PSTR("Shunt: %s\n"), Range == DC2mA ? "470R" : Range == DC20mA ? "47R" : Range == DC200mA ? "4R7" : "0R47");
    }

    // Berechnung des DAC-Wertes f¸r Strom
    tmpDAC = (int32_t)(wCurrent * DCAmpMod / DACLSBI[Range] + 0.5) + Params.DACIOffsets[Range];
    if (tmpDAC > DACMax)
    {
        tmpDAC = DACMax;
    }
    else if (tmpDAC < 0)
    {
        tmpDAC = 0;
    }

    sreg = SREG;
    cli();
    DACRawI = tmpDAC;
    RangeI = Range;
    SREG = sreg;

//*** Conversion for Voltage *************************************************

    if ( ArbActive == 0 )
    {

        if (LockRangeU == 255)
        {
            Range = 0;
            if (wVoltage > Params.MaxVoltage[0])
            {
                Range = 1;
            }
        }
        else
        {
            Range = LockRangeU;
        }
        if (Range != RangeU)
        {
            DCVoltMod = 1; // Prozent-Faktor r¸cksetzen
        }
        // Berechnung des DAC-Wertes f¸r Spannung
        tmpDAC = (int32_t)(wVoltage * DCVoltMod / DACLSBU[Range] + 0.5) + Params.DACUOffsets[Range];
        if (tmpDAC > DACMax)
        {
            tmpDAC = DACMax;
        }
        else if (tmpDAC < 0)
        {
            tmpDAC = 0;
        }

        // Berechnung des DAC-Wertes f¸r Spannung f¸r Ripple-Low U = (Uo* (100-RippleMod))/100
        rippleDAC = (int32_t)(wVoltage * DCVoltMod * (100 - Params.RippleMod) / (100.0 * DACLSBU[Range]) + 0.5) + Params.DACUOffsets[Range];
        if (rippleDAC > DACMax)
        {
            rippleDAC = DACMax;
        }
        else if (rippleDAC < 0)
        {
            rippleDAC = 0;
        }


        if ( RippleActive )
        {
            RippleVoltage = wVoltage * DCVoltMod * Params.RippleMod / 100.0; // to switch the relay accordingly.
        }
        else
        {
            RippleVoltage = 0.0;
        }

        sreg = SREG;
        cli();
        DACRawU = tmpDAC;
        DACRawURipple = rippleDAC;
        TmrRippleMod = Params.RippleMod;
        TmrRippleOn = Params.RippleOn;
        TmrRippleOff = Params.RippleOff;
        RangeU = Range;
        SREG = sreg;
    }
    else
    {
//*** Begin of Code for Arbitrary Mode *************************************************

        // Option Multiple Sequence ROM_Array
        // Retrieve pointer to code for selected arbitray sequence
        memcpy_P(&ArbArrayV_Ptr, &ArbArrayV[ArbSelect], sizeof(const double *));
        memcpy_P(&ArbArrayT_Ptr, &ArbArrayT[ArbSelect], sizeof(const uint16_t *));

        RelativeMaxVoltage0 = Params.MaxVoltage[0] / wVoltage;

        if (LockRangeU == 255)
        {
            Range = 0;              // this section was for absolute values, can be significantly reduced for relative values 0.0-1.0

            Index = 0;
            do      // for all voltage values of the array:
            {
                //      - find the highest voltage for correct Range setting
                //      - find the lowest voltage for correct calculation of relay switching

                if ( ArbActive == 1 ) // ROM mode
                {
                    if  ( Index >= ARBINDEXMAX ) break;

                    // Option Multiple Sequence ROM_Array, pick from  selected Sequence
                    memcpy_P(&tmpArbV, &ArbArrayV_Ptr[Index], sizeof(double));
                    memcpy_P(&tmpArbT, &ArbArrayT_Ptr[Index], sizeof(uint16_t));

                }
                else // ( ArbActive == 2 ) // RAM mode
                {
                    if  ( Index >= ARBINDEXMAXRAM ) break;

                    // Option Single Sequence RAM-Array
                    tmpArbV = ArbV_RAM[Index + ArbRAMOffset];
                    tmpArbT = ArbT_RAM[Index + ArbRAMOffset];
                }

 /*             // to be activated if output shall remain 100% during update of the Arbitrary configuration. However, seeing the update in the waveform is actually fun, too.
                if (ArbUpdateMode != 0)                 // Workaround for the situations when the array is being modified.
                {
                    ArbMinVoltage = 1.0;
                    tmpArbT = 0;
                }
 */

                if ( tmpArbV > RelativeMaxVoltage0 )
                {
                    Range = 1;
                }

                if (( Index == 0 ) || ( tmpArbV < ArbMinVoltage ))
                {
                    ArbMinVoltage = tmpArbV;  // still relative value here
                }

                Index++;

            }
            while ( tmpArbT != 0) ;

            ArbMinVoltage *= wVoltage;  // now make absolute value
        }
        else  // does it make any sense to use the fixed range here? *** not exactly ... tbd
        {
            Range = LockRangeU;
        }

        if (Range != RangeU)  // does it make any sense to use the VoltMod here? Yes, to scale predefined sequences 100% .. 0%
        {
            DCVoltMod = 1; // Prozent-Faktor r¸cksetzen
        }

        Index = 0;
        do
        {

            if ( ArbActive == 1 ) // ROM mode
            {
                if  ( Index >= ARBINDEXMAX ) break;

                // Option Multiple Sequence ROM_Array, pick from  selected Sequence
                memcpy_P(&tmpArbV, &ArbArrayV_Ptr[Index], sizeof(double));
                memcpy_P(&tmpArbT, &ArbArrayT_Ptr[Index], sizeof(uint16_t));

            }
            else // ( ArbActive == 2 ) // RAM mode
            {
                if  ( Index >= ARBINDEXMAXRAM ) break;

                // Option Single Sequence RAM-Array
                tmpArbV = ArbV_RAM[Index + ArbRAMOffset];
                tmpArbT = ArbT_RAM[Index + ArbRAMOffset];
            }

            if ((Index == 0) && (tmpArbT == 0))     // Workaround for "empty" array (first time value 0) ISR can't handle only one parameter
            {
                tmpArbV = 1.0;
            }
            /* to be activated
                    if (ArbUpdateMode != 0)                 // Workaround for the situations when the array is being modified.
                    {
                        tmpArbV = 1.0;
                        tmpArbT = 0;
                    }
            */
            if  ( Index >= ARBINDEXMAX ) break;
            // Berechnung des DAC-Wertes f¸r Spannung
            tmpDAC = (int32_t)(tmpArbV * wVoltage * DCVoltMod / DACLSBU[Range] + 0.5) + Params.DACUOffsets[Range];
            if (tmpDAC > DACMax)
            {
                tmpDAC = DACMax;
            }
            else if (tmpDAC < 0)
            {
                tmpDAC = 0;
            }
            if (((Index == 0) && (tmpArbT == 0)) /* || (ArbUpdateMode != 0)*/)      // Workaround for "empty" array (first time value 0) ISR can't handle only one parameter
            {
                sreg = SREG;
                cli();
                ArbDAC[0] = ArbDAC[1] = tmpDAC;
                ArbT[0] = ArbT[1] = tmpArbT;
                RangeU = Range;
                SREG = sreg;
            }
            else                                    // Regular process
            {
                sreg = SREG;
                cli();
                ArbDAC[Index] = tmpDAC;
                ArbT[Index] = tmpArbT;
                RangeU = Range;
                SREG = sreg;
            }

            Index++;

        }
        while ( tmpArbT != 0) ;

        sreg = SREG;
        cli();
        ArbTrigger = ArbRepeat;
        ArbDelayISR = ArbDelay;
        RangeU = Range;
        SREG = sreg;

    }

//*** End of Code for Arbitrary Mode *************************************************

}

void jobFaultCheck(void)
{
    double tmpVolt;
    static uint8_t count = 0;

    if (++count >= 20)   // 20*100ms = 2s
    {
        Temperature = GetLM75Temp();

        if (Temperature > 80)
        {
            if (!Status.OverTemp)
            {
                DPRINT(PSTR("%-10lu LM75: %.1f∞C\n"), GetTicker(), Temperature);
                SerPrompt(OvlErr, Status.u8);
            }
            Status.OverTemp = 1;
        }
        else if (Temperature < 70)
        {
            if (Status.OverTemp)
            {
                Flags.PwrInRange = 0;
            }
            Status.OverTemp = 0;
        }
        count=0;
    }

    // Eingangsspannung berechnen
    tmpVolt = GetPowerIn();

    uint8_t lowInput = (tmpVolt < 7.0);

    if (xMeanVoltage > (tmpVolt - 2.0))     // Transistor Q12 may be broken?
    {
        if (!Status.OverVolt && !lowInput)
        {
            DPRINT(PSTR("%-10lu Vout: %.2f, Vin: %.2f\n"), GetTicker(), xMeanVoltage, tmpVolt);
            SerPrompt(FaultErr, Status.u8);
        }
        Status.OverVolt = 1;
    }
    else
    {
        if (Status.OverVolt)
        {
            Flags.PwrInRange = 0;
        }
        Status.OverVolt = 0;
    }


    if  (Flags.RelayState == 2)             // C-Firmware has delays between switching relays, so only check when relays are on
    {                                       // FUSEBLWN is a "message" only an will not trigger any relay action
        if ( !Params.Options.SuppressFuseblowMsg )
        {
            if ( !Status.FuseBlown && lowInput)
            {
                SerPrompt(FuseErr, Status.u8);
            }
            Status.FuseBlown = lowInput;
        }
        else
        {
            Status.FuseBlown = 0;
        }
    }
}

void jobSwitchRelay(void)
{
    static uint8_t RelayTimer = 0;
    static uint8_t CurrentModeCounter = 0;

    if (PIND & (1<<PD4))
    {
        if (CurrentModeCounter > 0)
        {
            CurrentModeCounter--;
        }
    }
    else
    {
        if (CurrentModeCounter < 30)
        {
            CurrentModeCounter++;
        }
    }
    if (Status.CurrentMode && CurrentModeCounter < 10)
    {
        Status.CurrentMode = 0;
        PORTD &= ~(1<<PD3);     // voltage LED on, current LED off
    }
    if (!Status.CurrentMode && CurrentModeCounter > 20)
    {
        Status.CurrentMode = 1;
        PORTD |= (1<<PD3);      // voltage LED off, current LED on
    }

    if  (!Status.CurrentMode)
    {
        if (wVoltage > Params.RelayVoltage )                                // normal operation, switch to higher input voltage
        {
            Flags.PwrInRange = 1;
        }
        else if (wVoltage + 0.5 < Params.RelayVoltage )                     // normal operation, kick down to lower input voltage with 0.5V hysteresis
        {
            Flags.PwrInRange = 0;
        }
    }
                                                                            // Philosophy: kick down if regular (max<=>min) Voltage fits below RelayVoltage
    if (xMeanVoltage + 0.5 < Params.RelayVoltage - ( ArbActive ? (wVoltage - ArbMinVoltage) : RippleVoltage ) )  // kick-down to lower input voltage in overcurrent and ripple mode 0.5V hysteresis
    {                                                                       // depending on mode: minimum voltage of ripple or arbitrary
        Flags.PwrInRange = 0;
    }                                                                       // to be adapted for arbitrary mode.

    /* Original Implementation in DCG 2.842 / cm / c't
        DCVoltIntegrated:=(DCVoltIntegrated+Param)/2;
        if (DCVolt>RelaisVoltHigh) and (not OverloadFlag) then
          RelaisState:=high;
        endif;
        if DCVoltIntegrated>RelaisVoltHigh then
          RelaisState:=high;
        endif;
        if DCVoltIntegrated<RelaisVoltLow then
          RelaisState:=low;
        endif;
    */
    /* Modified Implementation in DCG 2.91 / cm / c't
        DCVoltIntegrated:=(DCVoltIntegrated+Param)/2;
        if (DCVolt>RelaisVoltHigh) and (not OverloadFlag) then
          RelaisState:=high;
        endif;
        if DCVoltIntegrated<(RelaisVoltLow-RippleVoltage) then
          RelaisState:=low;
        endif;
    */

    /* Idea of switching the relay is basically:
        - if not in current limitation mode and high output voltages: high input voltage
        - if the >>measured<< voltage is below the switching limit (and a hysteresis), then switch to lower input voltage to avoid high power dissipation
        - only if the overcurrent situation is removed, then it will be switched back to the higher input voltage.
          Note: this was different with the old implementation: The switch-back to the higher voltage would be done at any time the output voltage exceeds the threshold.

        - if in ripple mode: the measured voltage can be lower, i.e. as low as set voltage minus ripple voltage, WITHOUT switching to the lower input voltage
        - in DCG2 firmware, with high ripple time longer than 20ms, the high voltage can be reliably measured, so there would be no need to keep the input high
        - however, below 20ms, the measured result is somehow unreliable and therefore the calculation with the ripple voltage is needed.

        - since in Ripple mode the threshold is lower, the regular down-kick to the lower input voltage has to be handled separately. This is new in C-Firmware.
    */


    if (RelayTimer)
    {
        RelayTimer--;
    }


    switch(Flags.RelayState)
    {
        case 0: // off
            if (!Status.OverVolt && !Status.OverTemp)
            {
                if(Flags.PwrInRange)
                {
                    DPRINT(PSTR("%-10lu Relay 1: on\n"), GetTicker());
                    PORTB |= (1<<PB2);  // Relay 1 on
                }
                else
                {
                    DPRINT(PSTR("%-10lu Relay 2: on\n"), GetTicker());
                    PORTB |= (1<<PB3);  // Relay 2 on
                }
                RelayTimer = 2; // 8ms
                Flags.RelayState = 1;
            }
            break;


        case 1: // Wait after switching
            if (RelayTimer == 0)
            {
                if (PORTB & ((1<<PB3)|(1<<PB2)))
                {
                    // at least one relay is on
                    Flags.RelayState = 2;
                }
                else
                {
                    // both relays are off
                    if (Status.OverVolt || Status.OverTemp)
                    {
                        Flags.RelayState = 0;
                    }
                    else
                    {
                        if(Flags.PwrInRange)
                        {
                            DPRINT(PSTR("%-10lu Relay 1: on\n"), GetTicker());
                            PORTB |= (1<<PB2);  // Relay 1 on
                        }
                        else
                        {
                            DPRINT(PSTR("%-10lu Relay 2: on\n"), GetTicker());
                            PORTB |= (1<<PB3);  // Relay 2 on
                        }
                        RelayTimer = 2; // 8ms
                        /*
                        //debug

                            printf_P(PSTR("#%d:wVoltage: %.3f\n"), SlaveCh, wVoltage);
                            printf_P(PSTR("#%d:xVoltage: %.3f\n"), SlaveCh, xVoltage);
                            printf_P(PSTR("#%d:xMeanVoltage: %.3f\n"), SlaveCh, xMeanVoltage);

                            printf_P(PSTR("#%d:RippleVoltage: %.3f\n"), SlaveCh, RippleVoltage);
                            printf_P(PSTR("#%d:ArbMinVoltage: %.3f\n"), SlaveCh, ArbMinVoltage);
                            printf_P(PSTR("#%d:DCVoltMod: %.2f\n"), SlaveCh, DCVoltMod);
                            printf_P(PSTR("#%d:Params.RippleMod: %d%%\n"), SlaveCh, Params.RippleMod);
                             if(Flags.PwrInRange)
                            {
                                printf_P(PSTR("#%d:Relay 1 on\n"), SlaveCh);
                            }
                            else
                            {
                                printf_P(PSTR("#%d:Relay 2 on\n"), SlaveCh);
                            }

                        //debug
                        */
                    }
                }
            }
            break;

        case 2: // on
            if (Status.OverVolt || Status.OverTemp)
            {
#ifdef DEBUG
                if ((PORTB & ((1<<PB3)|(1<<PB2))) == ((1<<PB3)|(1<<PB2)))
                {
                    DPRINT(PSTR("%-10lu Relay 1,2: off\n"), GetTicker());
                }
                else if (PORTB & (1<<PB2))
                {
                    DPRINT(PSTR("%-10lu Relay 1: off\n"), GetTicker());
                }
                else if(PORTB & (1<<PB3))
                {
                    DPRINT(PSTR("%-10lu Relay 2: off\n"), GetTicker());
                }
#endif
                PORTB &= ~(1<<PB2); // Relay 1 off
                PORTB &= ~(1<<PB3); // Relay 2 off
                RelayTimer = 2; // 8ms
                Flags.RelayState = 1;
            }
            else if (Flags.PwrInRange)
            {
                if (PORTB & (1<<PB3))
                {
                    // Relay 2 is on
                    DPRINT(PSTR("%-10lu Relay 2: off\n"), GetTicker());
                    PORTB &= ~(1<<PB3); // Relay 2 off
                    RelayTimer = 2;
                    Flags.RelayState = 1;
                }

            }
            else
            {
                if (PORTB & (1<<PB2))
                {
                    // Relay 1 is on
                    DPRINT(PSTR("%-10lu Relay 1: off\n"), GetTicker());
                    PORTB &= ~(1<<PB2); // Relay 1 off
                    RelayTimer = 2; // 8ms
                    Flags.RelayState = 1;
                }
            }
            break;
    }
}

void SetActivityTimer(uint8_t Value)
{
    if (Value > ActivityTimer)
    {
        ActivityTimer = Value;
        PORTD &= ~(1<<PD2);             // Activity LED an
    }
}

void jobActivityTimer(void)
{
    if (ActivityTimer)
    {
        ActivityTimer--;
        if (ActivityTimer == 0)
        {
            PORTD |= (1<<PD2);          // Activity LED aus
        }
    }
}

//void __attribute__((noreturn)) main(void)
int main(void)
{
    uint8_t i;
    uint8_t StartTimer = 0;
    uint8_t ToggleTimer = 0;

    // Ports initialisieren
    DDRA = 0;
    PORTA = (1<<PA1)|(1<<PA0);

    DDRB = (1<<DDB7)|(1<<DDB5)|(1<<DDB4)|(1<<DDB3)|(1<<DDB2)|(1<<DDB1)|(1<<DDB0);
    PORTB = (1<<PB7)|(1<<PB6)|(1<<PB4)|(1<<PB1)|(1<<PB0);

    DDRC = (1<<DDC7)|(1<<DDC6)|(1<<DDC5)|(1<<DDC4)|(1<<DDC3)|(1<<DDC2);
#ifdef DUAL_DAC
    PORTC = (1<<PC5)|(1<<PC4)|(1<<PC3)|(1<<PC2)|(1<<PC1)|(1<<PC0);
#else
    PORTC = (1<<PC3)|(1<<PC2)|(1<<PC1)|(1<<PC0);
#endif

    DDRD = (1<<DDD3)|(1<<DDD2);
    PORTD = (1<<PD7)|(1<<PD6)|(1<<PD5)|(1<<PD2);


//  Test setup for debugging output on Address bit3
//DDRD = (1<<DDD7)|(1<<DDD3)|(1<<DDD2);

//PORTD |= (1<<PD7);  // Test output on PD7
//PORTD &= ~(1<<PD7); // Test output on PD7

    // Interrupts freigeben
    sei();

    // Timer initialisieren
    InitTimer();

    // Warten bis C's vom MAX232 geladen sind
    wait_us(20000);

    SlaveCh = ((uint8_t)~PIND) >> 5;

    // I2C initialisieren
    initI2C();

    // Init LCD
    if (Lcd_Init())
    {
        Flags.LCDPresent = 1;
    }


    // Parameter initialisieren
    if (0xaa55 == eeprom_read_word(&eepParams.Initialised))
    {
        eeprom_read_block(&Params, &eepParams, sizeof(Params));

        eeprom_read_block(ArbV_RAM, ArbV_EEP, sizeof(ArbV_RAM));
        eeprom_read_block(ArbT_RAM, ArbT_EEP, sizeof(ArbT_RAM));
    }
    else
    {
        Lcd_Write_P(0, 0, strlen_P(InitEEPStr_P), InitEEPStr_P);
        init_Arb_RAMarray();                                            // write all data first
        eeprom_write_block(ArbV_RAM, ArbV_EEP, sizeof(ArbV_RAM));
        eeprom_write_block(ArbT_RAM, ArbT_EEP, sizeof(ArbT_RAM));
        eeprom_write_byte(&StartParamSet,0);

        eeprom_write_block(&Params, &eepParams, sizeof(eepParams));     // last word of Params is the Initialized indicator.
    }

//init_Arb_RAMarray();

    // UART initialisieren
    Uart_Init_UBRR(Params.SerBaudReg);

    // printf auf UART verbiegen
    stdout = &mystdout;


    // Bootstring on LCD
    char s[9];

    Lcd_Write_P(0, 0, strlen_P(VersStrShort_P), VersStrShort_P);
    sprintf_P(s, PSTR("Adr  %-3d"), SlaveCh);

    Lcd_Write(0, 1, 8, s);


    PORTD &= ~(1<<PD2);         // Activity LED an

    wait_us(1000000);           // 1sec warten

    for (i = 0; i < SlaveCh; i++)
    {
        PORTD |= (1<<PD2);      // Activity LED aus
        wait_us(150000);
        PORTD &= ~(1<<PD2);     // Activity LED ein
        wait_us(150000);
    }
    PORTD |= (1<<PD2);          // Activity LED aus

    Status.u8 = 0;

    InitScales();

    LockRangeU = InitLockRangeU;
    LockRangeI = InitLockRangeI;

    DCVoltMod = 1;              // Faktor, Parameter aber in Prozent
    DCAmpMod = 1;
    wVoltage = 0;
    wCurrent = 0;               // F¸r den Einschaltvorgang mit Q3 (PC7-->1) Sollstrom auf Null einstellen
    ArbActive = 0;

    SetLevelDAC();              // Anfangswerte
    wait_us(4000);              // warten, daﬂ die ersten DAC Werte, Strom und Spannung = 0 ausgegeben wurden

    PORTC |= (1<<PC7);          // Outputenable == Ausgang ein
    wait_us(4000);              // warten, daﬂ die Regelschleife eingeschwungen ist

    wCurrent = Params.InitCurrent;
    SetLevelDAC();              // Soll-Strom zuschalten
    wait_us(4000);              // noch ein wenig abwarten

    ParseGetParam(254);         // DCG-String auf dem Bus ausgeben


    // Timer Starten
    StartTimers();

    set_sleep_mode(SLEEP_MODE_IDLE);

    while(1)
    {
        if (TestAndResetTimerOV(TIMER_100MS))
        {
            // Funktionen mit 100ms Periode
            CalcAmpWattHours();
            jobFaultCheck();

            ToggleTimer ++;
            if (ToggleTimer == 20)
            {
                // toggling display every 2 seconds back and forth
                ToggleDisplay = 0x01;
            }
            else if (ToggleTimer >= 40)
            {
                ToggleTimer = 0;
                ToggleDisplay = 0x00;
            }
        }
        if (TestAndResetTimerOV(TIMER_50MS))
        {
            // Funktionen mit 50ms Periode
            jobPanel();
        }
        if (TestAndResetTimerOV(TIMER_10MS))
        {
            // Funktionen mit 10ms Periode
        }
        if (TestAndResetTimerOV(TIMER_4MS))
        {
            jobGetValues();

            // Funktionen mit 4ms Periode

            if (StartTimer >= 20)
            {
                jobParseData();
            }

            if (StartTimer < 255)
            {
                if (StartTimer == 4)
                {
                    wVoltage = Params.InitVoltage;

                    wStartParamSet = eeprom_read_byte(&StartParamSet);

                    if ( 0 != wStartParamSet)
                    {
                        if ((wStartParamSet - 1) == RecallUserParamSet(wStartParamSet - 1, RECALL))
                            ActiveParamSet = wStartParamSet;
                    }

                    ArbActive = ARBACTIVESTART;  // as long as Arbitrary mode is NOT included in Userparams

                    CheckLimits();
                    SetLevelDAC();              // Anfangswerte
                    SendTrackCmd();
                    Flags.PwrInRange = 0;
                    ErrCount = 0;
                }
                else if (StartTimer == 40)
                {
                    jobFaultCheck();
                }
                StartTimer++;
            }

            jobSwitchRelay();
            jobActivityTimer();
        }
        sleep_enable();
        sleep_cpu();
        sleep_disable();
    }
}

