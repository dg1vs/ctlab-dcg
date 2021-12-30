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
#include <avr/pgmspace.h>

#include <stdio.h>

#include "dcg.h"
#include "Parser.h"
#include "timer.h"

#define NDEBUG
#include "debug.h"


const PROGMEM char Mnemonics[][4] =
{
    {'n','o','p',  0},
    {'v','a','l',  0},
    {'d','c','v',  0},    // Spannung U soll in V
    {'d','c','a',  1},    // Strom I soll in A, mA od. uA
    {'m','a','h',  7},    // Messwert kumulierter Strom in Ah (wird durch MAH=0! auf 0 gesetzt)
    {'m','w','h',  8},    // Messwert kumulierte Energie in Wh wird durch MWH=0! auf 0 gesetzt)
    {'m','s','v',  10},   // Meßwert U ist in V
    {'m','s','a',  11},   // Meßwert I ist in A, mA od. uA
    {'m','s','w',  18},   // tatsächlich abgegebene Leistung in Watt
    {'p','c','v',  20},   // Prozentwert für Ausgangsspannung
    {'p','c','a',  21},   // Prozentwert für Ausgangsstrom
    {'r','o','n',  27},   // Ripple on in ms, gerade Werte
    {'r','o','f',  28},   // Ripple off in ms, gerade Werte
    {'r','i','p',  29},   // Ripple in Prozent
    {'o','u','t',  40},   // Output On/Off
    {'r','a','w',  50},   // Rohdaten DAC/ADC
    {'d','s','p',  89},   // Inkrementalgeber Impulse pro Rastpunkt
    {'a','l','l',  99},   // Meßwerte
    {'o','f','s', 100},   // Offset DAC/ADC
    {'s','c','l', 200},   // Skalierung DAC/ADC
    {'o','p','t', 150},   // Parameter/Optionen
    {'w','e','n', 250},   // Write Enable für EEPROM
    {'e','r','c', 251},   // Fehlerzähler
    {'s','b','d', 252},   // Baudrateneinstellung
    {'i','d','n', 254},   // Identifizierung
    {'s','t','r', 255},   // Status
    { 0,  0,  0,    0}    // Terminator
};


#define PARAM_DOUBLE    0
#define PARAM_INT       1
#define PARAM_BYTE      2
#define PARAM_STR       3
#define PARAM_UINT16    4

#define SCALE_NONE      0
#define SCALE_A         1
#define SCALE_mA        2
#define SCALE_uA        3
#define SCALE_PROZ      4
#define SCALE_TEMP      5

typedef struct _PARAMTABLE
{
    uint8_t SubCh;
    union
    {
        double (*get_f_Function)(void);
        int32_t (*get_l_Function)(void);
        int16_t (*get_i_Function)(void);
        uint16_t (*get_u_Function)(void);
        uint8_t (*get_b_Function)(void);
        char* (*get_s_Function)(void);
        void (*doFunction)(struct _PARAMTABLE*);
        struct
        {
            union
            {
                double* f;
                int32_t* l;
                int16_t* i;
                uint16_t* u;
                uint8_t* b;
                const char* s;
            } ram;
            union
            {
                double* f;
                int32_t* l;
                int16_t* i;
                uint16_t* u;
                uint8_t* b;
                const char* s;
            } eep;
        } s;
    } u;
    uint8_t type        : 3; // Type of function/variable (int, double etc.)
    uint8_t scale       : 3;
    uint8_t rw          : 1; // 0 = read command (returns values to the caller), 1 = write (values to ctlab)
    uint8_t fct         : 2; // 0 = variable access, 1 = function pointer, 2 = special function
} PARAMTABLE;


//---------------------------------------------------------------------------------------------


int16_t GetADC2(void)
{
    return GetADC(2);
}


//---------------------------------------------------------------------------------------------

int16_t GetADC3(void)
{
    return GetADC(3);
}


//---------------------------------------------------------------------------------------------

int16_t GetADC4(void)
{
    return GetADC(4);
}


//---------------------------------------------------------------------------------------------

void GetAll(PARAMTABLE* ParamTable __attribute__((unused)))
{
    ParseGetParam(10);
    ParseGetParam(11);
    ParseGetParam(15);
}


//---------------------------------------------------------------------------------------------

void ReturnInput(PARAMTABLE* ParamTable __attribute__((unused)))
{
    printf_P(PSTR("%s\n"), SerInpStr);
}

/*
//---------------------------------------------------------------------------------------------

void ReturnIDN(PARAMTABLE* ParamTable __attribute__((unused)))

{
    PrintIDNstring();
}
*/

//---------------------------------------------------------------------------------------------

const PROGMEM PARAMTABLE SetParamTable[] =
{
    // Die Einträge müssen nach SubCh sortiert sein
    {.SubCh = 0,   .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &wVoltage, .eep.f = (double*)-1}},
    {.SubCh = 1,   .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_A,    .u.s = {.ram.f = &wCurrent, .eep.f = (double*)-1}},
    {.SubCh = 2,   .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_mA,   .u.s = {.ram.f = &wCurrent, .eep.f = (double*)-1}},
    {.SubCh = 3,   .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_uA,   .u.s = {.ram.f = &wCurrent, .eep.f = (double*)-1}},
    {.SubCh = 7,   .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &xAmpHours, .eep.f = (double*)-1}},
    {.SubCh = 8,   .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &xWattHours,.eep.f = (double*)-1}},
    {.SubCh = 10,  .rw = 0, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &xVoltage, .eep.f = (double*)-1}},
    {.SubCh = 11,  .rw = 0, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_A,    .u.s = {.ram.f = &xCurrent, .eep.f = (double*)-1}},
    {.SubCh = 12,  .rw = 0, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_mA,   .u.s = {.ram.f = &xCurrent, .eep.f = (double*)-1}},
    {.SubCh = 13,  .rw = 0, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_uA,   .u.s = {.ram.f = &xCurrent, .eep.f = (double*)-1}},
    {.SubCh = 15,  .rw = 0, .fct = 1, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.get_f_Function = GetPowerIn},
    {.SubCh = 16,  .rw = 0, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &xMeanVoltage, .eep.f = (double*)-1}},
    {.SubCh = 17,  .rw = 0, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_A,    .u.s = {.ram.f = &xMeanCurrent, .eep.f = (double*)-1}},
    {.SubCh = 18,  .rw = 0, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &xPower, .eep.f = (double*)-1}},
    {.SubCh = 20,  .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_PROZ, .u.s = {.ram.f = &DCVoltMod,  .eep.f = (double*)-1}},
    {.SubCh = 21,  .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_PROZ, .u.s = {.ram.f = &DCAmpMod,   .eep.f = (double*)-1}},
    {.SubCh = 27,  .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.RippleOn,  .eep.i = (int16_t*)-1}},
    {.SubCh = 28,  .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.RippleOff, .eep.i = (int16_t*)-1}},
    {.SubCh = 29,  .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.RippleMod, .eep.i = (int16_t*)-1}},
    {.SubCh = 40,  .rw = 1, .fct = 0, .type = PARAM_BYTE,   .scale = SCALE_NONE, .u.s = {.ram.b = &Params.OutputOnOff, .eep.b = (uint8_t*)-1}},
    {.SubCh = 50,  .rw = 0, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s.ram.u = &ADCRawU},
    {.SubCh = 51,  .rw = 0, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s.ram.u = &ADCRawI},
    {.SubCh = 52,  .rw = 0, .fct = 1, .type = PARAM_INT,    .scale = SCALE_NONE, .u.get_i_Function = GetADC2},
    {.SubCh = 53,  .rw = 0, .fct = 1, .type = PARAM_INT,    .scale = SCALE_NONE, .u.get_i_Function = GetADC3},
    {.SubCh = 54,  .rw = 0, .fct = 1, .type = PARAM_INT,    .scale = SCALE_NONE, .u.get_i_Function = GetADC4},
    {.SubCh = 70,  .rw = 0, .fct = 0, .type = PARAM_UINT16, .scale = SCALE_NONE, .u.s.ram.u = &DACRawU},
    {.SubCh = 71,  .rw = 0, .fct = 0, .type = PARAM_UINT16, .scale = SCALE_NONE, .u.s.ram.u = &DACRawI},
    {.SubCh = 89,  .rw = 1, .fct = 0, .type = PARAM_BYTE,   .scale = SCALE_NONE, .u.s = {.ram.b = &Params.IncRast, .eep.b = &eepParams.IncRast}},
    {.SubCh = 99,  .rw = 0, .fct = 2, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.doFunction = GetAll},
    {.SubCh = 100, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.DACUOffsets[0], .eep.i = &eepParams.DACUOffsets[0]}},
    {.SubCh = 101, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.DACUOffsets[1], .eep.i = &eepParams.DACUOffsets[1]}},
    {.SubCh = 102, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.DACIOffsets[0], .eep.i = &eepParams.DACIOffsets[0]}},
    {.SubCh = 103, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.DACIOffsets[1], .eep.i = &eepParams.DACIOffsets[1]}},
    {.SubCh = 104, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.DACIOffsets[2], .eep.i = &eepParams.DACIOffsets[2]}},
    {.SubCh = 105, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.DACIOffsets[3], .eep.i = &eepParams.DACIOffsets[3]}},
    {.SubCh = 110, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.ADCUOffsets[0], .eep.i = &eepParams.ADCUOffsets[0]}},
    {.SubCh = 111, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.ADCUOffsets[1], .eep.i = &eepParams.ADCUOffsets[1]}},
    {.SubCh = 112, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.ADCIOffsets[0], .eep.i = &eepParams.ADCIOffsets[0]}},
    {.SubCh = 113, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.ADCIOffsets[1], .eep.i = &eepParams.ADCIOffsets[1]}},
    {.SubCh = 114, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.ADCIOffsets[2], .eep.i = &eepParams.ADCIOffsets[2]}},
    {.SubCh = 115, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.ADCIOffsets[3], .eep.i = &eepParams.ADCIOffsets[3]}},
    {.SubCh = 150, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.InitVoltage, .eep.f = &eepParams.InitVoltage}},
    {.SubCh = 151, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.InitCurrent, .eep.f = &eepParams.InitCurrent}},
    {.SubCh = 152, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.GainPre, .eep.f = &eepParams.GainPre}},
    {.SubCh = 153, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.GainOut, .eep.f = &eepParams.GainOut}},
    {.SubCh = 154, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.GainI, .eep.f = &eepParams.GainI}},
    {.SubCh = 155, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.RefVoltage, .eep.f = &eepParams.RefVoltage}},
    {.SubCh = 156, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.MaxVoltage[1], .eep.f = &eepParams.MaxVoltage[1]}},
    {.SubCh = 157, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.RSense[0], .eep.f = &eepParams.RSense[0]}},
    {.SubCh = 158, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.RSense[1], .eep.f = &eepParams.RSense[1]}},
    {.SubCh = 159, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.RSense[2], .eep.f = &eepParams.RSense[2]}},
    {.SubCh = 160, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.RSense[3], .eep.f = &eepParams.RSense[3]}},
    {.SubCh = 161, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.MaxCurrent[0], .eep.f = &eepParams.MaxCurrent[0]}},
    {.SubCh = 162, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.MaxCurrent[1], .eep.f = &eepParams.MaxCurrent[1]}},
    {.SubCh = 163, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.MaxCurrent[2], .eep.f = &eepParams.MaxCurrent[2]}},
    {.SubCh = 164, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.MaxCurrent[3], .eep.f = &eepParams.MaxCurrent[3]}},
    {.SubCh = 165, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.ADCUfacs[0],   .eep.f = &eepParams.ADCUfacs[0]}},
    {.SubCh = 166, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.ADCUfacs[1],   .eep.f = &eepParams.ADCUfacs[1]}},
    {.SubCh = 167, .rw = 1, .fct = 0, .type = PARAM_BYTE,   .scale = SCALE_NONE, .u.s = {.ram.b = (uint8_t*)&Params.Options, .eep.b = (uint8_t*)&eepParams.Options}},
    {.SubCh = 168, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.MaxVoltage[0], .eep.f = &eepParams.MaxVoltage[0]}},
    {.SubCh = 170, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.RelayVoltage,  .eep.f = &eepParams.RelayVoltage}},
    {.SubCh = 171, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.FanOnTemp,     .eep.f = &eepParams.FanOnTemp}},
    {.SubCh = 172, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.RippleOn,  .eep.i = &eepParams.RippleOn}},
    {.SubCh = 173, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.RippleOff, .eep.i = &eepParams.RippleOff}},
    {.SubCh = 174, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &Params.RippleMod, .eep.i = &eepParams.RippleMod}},
    {.SubCh = 175, .rw = 1, .fct = 0, .type = PARAM_BYTE,   .scale = SCALE_NONE, .u.s = {.ram.b = &Params.OutputOnOff, .eep.b = &eepParams.OutputOnOff}},

    {.SubCh = 180, .rw = 1, .fct = 0, .type = PARAM_BYTE,   .scale = SCALE_NONE, .u.s = {.ram.b = &ActiveParamSet, .eep.b = (uint8_t*)-1}},
    {.SubCh = 181, .rw = 1, .fct = 0, .type = PARAM_BYTE,   .scale = SCALE_NONE, .u.s = {.ram.b = &wStartParamSet, .eep.b = &StartParamSet}},

    {.SubCh = 182, .rw = 1, .fct = 0, .type = PARAM_BYTE,   .scale = SCALE_NONE, .u.s = {.ram.b = &ArbActive, .eep.b = (uint8_t*)-1}},
    {.SubCh = 183, .rw = 1, .fct = 0, .type = PARAM_BYTE,   .scale = SCALE_NONE, .u.s = {.ram.b = &ArbSelect, .eep.b = (uint8_t*)-1}},
    {.SubCh = 184, .rw = 1, .fct = 0, .type = PARAM_BYTE,   .scale = SCALE_NONE, .u.s = {.ram.b = &ArbRepeat, .eep.b = (uint8_t*)-1}},
    {.SubCh = 185, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.i = &ArbDelay,  .eep.i = (int16_t*)-1}},

    {.SubCh = 186, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &ArbRAMtmpV, .eep.f = (double*)-1}},
    {.SubCh = 187, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.u = &ArbRAMtmpT, .eep.u = (uint16_t*)-1}},
    {.SubCh = 188, .rw = 1, .fct = 0, .type = PARAM_BYTE,   .scale = SCALE_NONE, .u.s = {.ram.b = &ArbUpdateMode, .eep.b = (uint8_t*)-1}},
    {.SubCh = 189, .rw = 1, .fct = 0, .type = PARAM_BYTE,   .scale = SCALE_NONE, .u.s = {.ram.b = &ArbSelectRAM,  .eep.b = (uint8_t*)-1}},

    {.SubCh = 200, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.DACUScales[0], .eep.f = &eepParams.DACUScales[0]}},
    {.SubCh = 201, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.DACUScales[1], .eep.f = &eepParams.DACUScales[1]}},
    {.SubCh = 202, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.DACIScales[0], .eep.f = &eepParams.DACIScales[0]}},
    {.SubCh = 203, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.DACIScales[1], .eep.f = &eepParams.DACIScales[1]}},
    {.SubCh = 204, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.DACIScales[2], .eep.f = &eepParams.DACIScales[2]}},
    {.SubCh = 205, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.DACIScales[3], .eep.f = &eepParams.DACIScales[3]}},
    {.SubCh = 210, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.ADCUScales[0], .eep.f = &eepParams.ADCUScales[0]}},
    {.SubCh = 211, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.ADCUScales[1], .eep.f = &eepParams.ADCUScales[1]}},
    {.SubCh = 212, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.ADCIScales[0], .eep.f = &eepParams.ADCIScales[0]}},
    {.SubCh = 213, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.ADCIScales[1], .eep.f = &eepParams.ADCIScales[1]}},
    {.SubCh = 214, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.ADCIScales[2], .eep.f = &eepParams.ADCIScales[2]}},
    {.SubCh = 215, .rw = 1, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_NONE, .u.s = {.ram.f = &Params.ADCIScales[3], .eep.f = &eepParams.ADCIScales[3]}},
    {.SubCh = 233, .rw = 0, .fct = 0, .type = PARAM_DOUBLE, .scale = SCALE_TEMP, .u.s = {.ram.f = &Temperature}},
    {.SubCh = 251, .rw = 1, .fct = 0, .type = PARAM_INT,    .scale = SCALE_NONE, .u.s = {.ram.u = &ErrCount, .eep.u = (uint16_t*)-1}},
    {.SubCh = 252, .rw = 1, .fct = 0, .type = PARAM_BYTE,   .scale = SCALE_NONE, .u.s = {.ram.b = &Params.SerBaudReg, .eep.b = &eepParams.SerBaudReg}},
    {.SubCh = 253, .rw = 0, .fct = 2, .type = PARAM_STR,    .scale = SCALE_NONE, .u.doFunction=ReturnInput},
    {.SubCh = 254, .rw = 0, .fct = 0, .type = PARAM_STR,    .scale = SCALE_NONE, .u.s = {.ram.s = VersStrLong}}
//  {.SubCh = 254, .rw = 0, .fct = 2, .type = PARAM_STR,    .scale = SCALE_NONE, .u.doFunction=ReturnIDN}
};


//---------------------------------------------------------------------------------------------

uint8_t ParseFindParamData(PARAMTABLE* Data, uint8_t SubCh)
{
    int16_t pos, min, max;

    min = 0;
    max = sizeof(SetParamTable) / sizeof(SetParamTable[0]) - 1;

    while (min <= max)
    {
        pos = (min + max) / 2;
        memcpy_P(Data, &SetParamTable[pos], sizeof(PARAMTABLE));

        if (SubCh == Data->SubCh)
        {
            // gefunden
            return 1;
        }

        if (SubCh > Data->SubCh)
        {
            min = pos + 1;
        }
        else
        {
            max = pos - 1;
        }
    }
    return 0;
}


//---------------------------------------------------------------------------------------------

void ParseGetParam(uint8_t SubCh)
{
    char fmt[20];
    PARAMTABLE ParamData;
    uint8_t fract_len = 4;
    union
    {
        double f;
        int32_t l;
        int16_t i;
        uint16_t u;
        uint8_t b;
        const char* s;
    } Data;

    if (SubCh == 255)
    {
        SerPrompt(NoErr, Status.u8);
        return;
    }

    if (!ParseFindParamData(&ParamData, SubCh))
    {
        CHECKPOINT;
        SerPrompt(ParamErr, 0);
        return;
    }

    if (ParamData.fct == 2)
    {
        // special function
        ParamData.u.doFunction(&ParamData);
        return;
    }

    if (ParamData.fct == 1)
    {
        // get function
        switch (ParamData.type)
        {
            case PARAM_DOUBLE:
                Data.f = ParamData.u.get_f_Function();
                break;

            case PARAM_INT:
                Data.i = ParamData.u.get_i_Function();
                break;

            case PARAM_UINT16:
                Data.u = ParamData.u.get_u_Function();
                break;

            case PARAM_BYTE:
                Data.b = ParamData.u.get_b_Function();
                break;

            case PARAM_STR:
                Data.s = ParamData.u.get_s_Function();
                break;

        }
    }
    else
    {
        // variable
        switch(ParamData.type)
        {
            case PARAM_DOUBLE:
                Data.f = *ParamData.u.s.ram.f;
                break;

            case PARAM_INT:
                Data.i = *ParamData.u.s.ram.i;
                break;

            case PARAM_UINT16:
                Data.u = *ParamData.u.s.ram.u;
                break;

            case PARAM_BYTE:
                Data.b = *ParamData.u.s.ram.b;
                break;

            case PARAM_STR:
                Data.s = ParamData.u.s.ram.s;
                break;
        }
    }

    // handle scaling of float variables
    if (ParamData.type == PARAM_DOUBLE)
    {
        switch (ParamData.scale)
        {
            case SCALE_PROZ:
                Data.f *= 1e2;
                fract_len = 2;
                break;

            case SCALE_A:
                fract_len = 6 - RangeI;
                break;

            case SCALE_mA:
                Data.f *= 1e3;
                fract_len = 3;
                break;

            case SCALE_uA:
                Data.f *= 1e6;
                fract_len = 0;
                break;

            case SCALE_TEMP:
                fract_len = 1;
                break;
        }
    }

    // print the parameters
    switch(ParamData.type)
    {
        case PARAM_DOUBLE:
            // create a format string, because avrgcc doesn't handle a variable length format specifier like this "%.*f"
            sprintf_P(fmt, PSTR("#%%d:%%d=%%.%df\n"), fract_len);
            printf(fmt, SlaveCh, SubCh, Data.f);
            break;

        case PARAM_INT:
            printf_P(PSTR("#%d:%d=%d\n"), SlaveCh, SubCh, Data.i);
            break;

        case PARAM_UINT16:
            printf_P(PSTR("#%d:%d=%u\n"), SlaveCh, SubCh, Data.u);
            break;

        case PARAM_BYTE:
            printf_P(PSTR("#%d:%d=%u\n"), SlaveCh, SubCh, Data.b);
            break;

        case PARAM_STR:
            printf_P(PSTR("#%d:%d=%s\n"), SlaveCh, SubCh, Data.s);
            break;

    }
}


//---------------------------------------------------------------------------------------------

void ParseSetParam(uint8_t SubCh, double Param)
{
    uint8_t tmpActiveParamSet = ActiveParamSet;
    uint8_t tmpwStartParamSet = wStartParamSet;

    static uint8_t ArbIndex = 0;
    static uint8_t ArbIndicator = 0;
    static uint8_t oldArbUpdateMode = 0;

    if (Status.Busy)
    {
        SerPrompt(BusyErr, 0);
        return;
    }
    if (SubCh == 250)
    {
        Status.EEUnlocked = 1;
    }
    else
    {
        PARAMTABLE Data;

        if (!ParseFindParamData(&Data, SubCh) || !Data.rw)
        {
            SerPrompt(ParamErr, 0);
            return;
        }

        if (Data.u.s.eep.f != (void*)-1 && !Status.EEUnlocked)
        {
            SerPrompt(LockedErr, 0);
            return;
        }

        switch(Data.type)
        {
            case PARAM_DOUBLE:
                switch(Data.scale)
                {
                    case SCALE_PROZ:
                        *Data.u.s.ram.f = Param / 1e2;
                        break;

                    case SCALE_mA:
                        *Data.u.s.ram.f = Param / 1e3;
                        break;

                    case SCALE_uA:
                        *Data.u.s.ram.f = Param / 1e6;
                        break;

                    default:
                    case SCALE_A:
                        *Data.u.s.ram.f = Param;
                        break;
                }

                break;

            case PARAM_BYTE:
                *Data.u.s.ram.b = (uint8_t)Param;
                break;

            case PARAM_INT:
                *Data.u.s.ram.i = (int16_t)Param;
                break;

        }

        if (Data.u.s.eep.f != (double*)-1)
        {
            uint8_t size;
            switch(Data.type)
            {
                case PARAM_DOUBLE:
                    size = sizeof(double);
                    break;

                case PARAM_BYTE:
                    size = sizeof(uint8_t);
                    break;

                case PARAM_INT:
                    size = sizeof(int16_t);
                    break;

                default:
                    size = 0;
            }

            CheckLimits();          // not only for, but including wStartParamSet

            if ((SubCh == 181) &&  (wStartParamSet != 0 )  ) // check if usersets are preset, default (0) is always present
            {
                if ((wStartParamSet - 1) != RecallUserParamSet(wStartParamSet - 1, CHECK))
                {
                    wStartParamSet = tmpwStartParamSet;     // if this parameter set has not been saved yet, restore previous value.
                }
            }

            eeprom_write_block(Data.u.s.ram.f, Data.u.s.eep.f, size);
            InitScales();

        }
        else if (SubCh == 180)          // ActiveParamSet
        {

            if (ActiveParamSet == 0)    // param vs ActiveParamSet
            {
                RecallDefaultParamSet();
                CheckLimits();
                SetLevelDAC();
                SendTrackCmd();
                PushParamSet();
            }
            else if ((ActiveParamSet > 0) && (ActiveParamSet <= USERSETS))
            {
                if (0 == RecallUserParamSet(ActiveParamSet - 1, RECALL))
                {
                    //only if not empty
                    // ActiveParamSet = Param;
                    CheckLimits();
                    SetLevelDAC();
                    SendTrackCmd();
                    PushParamSet();
                }
                else    // if paramset is empty
                {
                    ActiveParamSet = tmpActiveParamSet; // not successful, restore previous value
                                                        // don't do anything else, just ignore
                }
            }
            else if ((ActiveParamSet > USERSETS) && (ActiveParamSet <= 2*USERSETS))
            {
                ActiveParamSet -= USERSETS;
                SaveUserParamSet(ActiveParamSet - 1);
            }
        }

//*** Arbitrary Load Voltage Value = 186 *********************************

        else if ( (SubCh == 186) && (ArbUpdateMode == 1) )  // store Arbitrary Data to RAM Voltage array.
        {
            LIMIT_DOUBLE(&ArbRAMtmpV, 0.0, 1.0);
            ArbV_RAM[ArbIndex] = ArbRAMtmpV;
            ArbIndicator = 1;                               // indicator, that voltage value has been written to RAM
        }

//*** Arbitrary Load Time Value = 187 ************************************

        else if ( ( SubCh == 187) && (ArbUpdateMode == 1) ) // store Arbitrary Data to RAM time array.
        {
            if (ArbIndicator != 1)
            {
                ArbV_RAM[ArbIndex] = 1.0;                   // use default value, since no voltage value was set before this time value
            }

            LIMIT_UINT16(&ArbRAMtmpT, 0, 65000);
            ArbT_RAM[ArbIndex] = ArbRAMtmpT;

            ArbIndicator = 0;                               // indicator, that voltage/time value pair has been written to RAM

            if ( ArbIndex < ARBINDEXMAXRAM )
            {
                ArbIndex++;
            }
        }

//*** Arbitrary 188 ******************************************************

        else if ( SubCh == 188 )        // activate finalisation of Arbitrary Data to RAM incl. clearing of rest of memory.
        {
            if (Status.EEUnlocked == 1) // new sequence may be loaded
            {

                //*** ArbUpdateMode = 1 **************************************
                //*** Begin of Update ***

                if ( (oldArbUpdateMode == 0) && (ArbUpdateMode == 1))
                {
                    oldArbUpdateMode = ArbUpdateMode;
                    ArbIndex = 0;
                    ArbIndicator = 0;
                }

                //*** ArbUpdateMode = 2 **************************************
                //*** Finalisation of Update ***

                else if ( (oldArbUpdateMode == 1) && (ArbUpdateMode == 2))
                {
                    if ( (ArbIndicator == 0) && (ArbIndex > 0) )
                    {
                        ArbIndex--;
                    }

                    ArbT_RAM[ArbIndex] = 0;             // Ensure that the last Time value is zero, even there was another value loaded

                    if ( ArbIndex < ARBINDEXMAXRAM )
                    {
                        ArbIndex++;
                    }

                    while ( ArbIndex < ARBINDEXMAXRAM )
                    {
                        ArbV_RAM[ArbIndex] = 1.0;       // load the rest of the array with default value.
                        ArbT_RAM[ArbIndex] = 0;
                        ArbIndex++;
                    }
                    ArbIndex = 0;
                    ArbIndicator = 0;
                    ArbRAMOffset = get_SequenceStart_RAMarray(&ArbSelectRAM); // has to be called explicitly here because RAM array has chnaged
                    ArbUpdateMode = oldArbUpdateMode = 0;
                }

                //*** ArbUpdateMode = 3 **************************************
                //*** Save Arbitray Array to EEPROM ***

                else if ( (oldArbUpdateMode == 0) && (ArbUpdateMode == 3))
                {
                    eeprom_write_block(ArbV_RAM, ArbV_EEP, sizeof(ArbV_RAM));
                    eeprom_write_block(ArbT_RAM, ArbT_EEP, sizeof(ArbT_RAM));

                    ArbUpdateMode = oldArbUpdateMode = 0;
                }

                //*** ArbUpdateMode = 4 **************************************
                //*** Recall Arbitray Array from EEPROM ***

                else if ( (oldArbUpdateMode == 0) && (ArbUpdateMode == 4))
                {
                    eeprom_read_block(ArbV_RAM, ArbV_EEP, sizeof(ArbV_RAM));
                    eeprom_read_block(ArbT_RAM, ArbT_EEP, sizeof(ArbT_RAM));

                    ArbUpdateMode = oldArbUpdateMode = 0;
                }

                //*** ArbUpdateMode = unknown ********************************
                else
                {
                    ArbUpdateMode = oldArbUpdateMode;
                    SerPrompt(ParamErr, 0);
                }

            }
            else    // if not wen=1 then return to old mode
            {
                ArbUpdateMode = oldArbUpdateMode;
                SerPrompt(LockedErr, 0);
            }

        }


        Status.EEUnlocked = 0;
    }

    SerPrompt(NoErr, Status.u8);

    CheckLimits();
    SetLevelDAC();

    SendTrackCmd();
}


//---------------------------------------------------------------------------------------------

void SendTrackCmd(void)
{
    char ucBuffer[32];
    char* ss;
    uint8_t calcSum;



    if (TrackCh != 255)
    {

        sprintf_P(ucBuffer, PSTR("%d:0=%.6f"), TrackCh, wVoltage);
        ss = ucBuffer;
        calcSum = 0;

        // calculate the checksum from command data
        while (*ss != 0)
        {
            calcSum ^= *ss++;
        }
        printf_P(PSTR("%s$%02x\n"), ucBuffer, calcSum);


        sprintf_P(ucBuffer, PSTR("%d:1=%.6f"), TrackCh, wCurrent);
        ss = ucBuffer;
        calcSum = 0;

        // calculate the checksum from command data
        while (*ss != 0)
        {
            calcSum ^= *ss++;
        }
        printf_P(PSTR("%s$%02x\n"), ucBuffer, calcSum);

    }
}

void SendTrackOnOff(void)
{
    char ucBuffer[32];
    char* ss;
    uint8_t calcSum;



    if (TrackCh != 255)
    {
        sprintf_P(ucBuffer, PSTR("%d:40=%u"), TrackCh, Params.OutputOnOff);
        ss = ucBuffer;
        calcSum = 0;

        // calculate the checksum from command data
        while (*ss != 0)
        {
            calcSum ^= *ss++;
        }
        printf_P(PSTR("%s$%02x\n"), ucBuffer, calcSum);
    }    
}



