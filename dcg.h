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

#ifndef __DCG_H__
#define __DCG_H__

#include <inttypes.h>
#include <avr/eeprom.h>

#include "config.h"

#define DC2mA       0
#define DC20mA      1
#define DC200mA     2
#define DC2000mA    3

//Number of UserSets
#define USERSETS    3

#define CHECK       0
#define RECALL      1

//Select Units for Display
#define AMP         10
#define WATT        12

#define VOLT        13
#define AMPHOUR     14
#define WATTHOUR    15

#define SECOND      20
#define PERCENT     21
#define BLANK       22

typedef struct
{
    uint8_t RelayState;

    uint8_t PwrInRange      : 1;

    uint8_t LCDPresent      : 1;

} FLAGS;

typedef union
{
    uint8_t u8;
    struct
    {
        uint8_t Unused      : 1;
        uint8_t FuseBlown   : 1;
        uint8_t OverVolt    : 1;
        uint8_t OverTemp    : 1;
        uint8_t EEUnlocked  : 1;
        uint8_t CurrentMode : 1;
        uint8_t UserSRQ     : 1;
        uint8_t Busy        : 1;
    };
} STATUS;

typedef struct
{
    uint8_t DAC16Present        : 1;   // OPT17 / Bit0 - set if LTC1655 rather than LTC1257 is used as U7
    uint8_t ADC16Present        : 1;   // OPT17 / Bit1 - set if LTC1864 is used as U3
    uint8_t DCP_Present         : 1;   // OPT17 / Bit2 - Relay Switching in case of DCP - is ignored in C-Firmware (relay switching done anyway)
    uint8_t SuppressFuseblowMsg : 1;   // OPT17 / BiT3 - Suppress "Fuseblow" message, e.g. in case HCP45 is connected (DCG input voltages can get very low even during normal operation)
} OPTIONS;

typedef struct
{
    int16_t DACUOffsets[2];
    double DACUScales[2];
    int16_t DACIOffsets[4];
    double DACIScales[4];
    int16_t ADCUOffsets[2];
    double ADCUScales[2];
    int16_t ADCIOffsets[4];
    double ADCIScales[4];
    double InitVoltage;
    double InitCurrent;
    double GainPre;
    double GainOut;
    double GainI;
    double RefVoltage;
    double MaxVoltage[2];
    double RSense[4];
    double MaxCurrent[4];
    double ADCUfacs[2];
    OPTIONS Options;
    double RelayVoltage;
    double FanOnTemp;
    int16_t RippleOn;
    int16_t RippleOff;
    int16_t RippleMod;
    uint8_t IncRast;
    uint8_t SerBaudReg;
    uint8_t TrackChSave;
    double GainPwrIn;
    uint8_t LockRangeU;
    uint8_t LockRangeI;
    uint16_t Initialised;
    uint8_t OutputOnOff;
} PARAMS;

typedef struct
{
    double InitVoltage;
    double InitCurrent;
//  double InitPower;
//  double InitOhm;
//  uint8_t ModeRange;
//  double LimitCurrent;
    int16_t RippleOn;
    int16_t RippleOff;
    int16_t RippleMod;
    uint8_t LockRangeU;
    uint8_t LockRangeI;
    uint16_t Initialised;
} USERPARAMS;


extern FLAGS Flags;
extern STATUS Status;
extern PARAMS Params;
extern PARAMS eepParams EEMEM;

extern double DCVoltMod;
extern double DCAmpMod;
extern double Temperature;
extern double xVoltage;
extern double xVoltageLow;
extern double xCurrent;
extern double xCurrentLow;
extern double xPower;
extern double xPowerTot;
extern double xAmpHours;
extern double xWattHours;

extern double xMeanVoltage;
extern double xMeanCurrent;

extern double wVoltage;
extern double wCurrent;

extern uint16_t ADCRawU;
extern uint16_t ADCRawULow;
extern uint16_t ADCRawI;
extern uint16_t ADCRawILow;
extern uint16_t DACRawU;
extern uint16_t DACRawI;

extern uint16_t DACRawURipple;

extern int16_t TmrRippleMod;
extern int16_t TmrRippleOn;
extern int16_t TmrRippleOff;

extern uint8_t SlaveCh;
extern uint8_t TrackCh;
extern uint16_t ErrCount;

extern uint8_t RangeI;
extern uint8_t RangeU;

extern uint8_t LockRangeI;
extern uint8_t LockRangeU;

extern int16_t wRippleOn;
extern int16_t wRippleOff;
extern int16_t wRippleMod;

extern uint8_t ToggleDisplay;

extern uint8_t ActiveParamSet;
extern uint8_t StartParamSet EEMEM;
extern uint8_t wStartParamSet;


extern const char VersStrLong[];
extern char SerInpStr[];

extern uint16_t RippleActive;

//*** Arbitrary Mode variables/constants/functions *********************

extern uint16_t   ArbDAC[];
extern uint16_t   ArbT[];
extern const char PROGMEM* const PROGMEM ArbArrayL[];

extern uint8_t  ArbActive;
extern uint8_t  ArbSelect;

extern uint8_t  ArbRepeat;
extern uint8_t  ArbTrigger;

extern int16_t  ArbDelay;
extern int16_t  ArbDelayISR;


extern uint8_t ArbSelectRAM;
extern uint8_t ArbRAMOffset;

#define ARBINDEXMAXRAM 75

extern double   ArbV_RAM[ARBINDEXMAXRAM];
extern uint16_t ArbT_RAM[ARBINDEXMAXRAM];

extern double   ArbV_EEP[ARBINDEXMAXRAM]  EEMEM;
extern uint16_t ArbT_EEP[ARBINDEXMAXRAM]  EEMEM;

extern uint8_t  ArbUpdateMode;
extern double   ArbRAMtmpV;
extern uint16_t ArbRAMtmpT;

extern uint8_t get_SequenceStart_RAMarray(uint8_t*);

//*** Arbitrary Mode variables/constants/functions *********************



// dcg.c //////////////////////////////////////////////////
double GetPowerIn(void);
void InitScales(void);
void SetLevelDAC(void);
void CheckLimits(void);
uint8_t CalcRangeI(double);
void SetActivityTimer(uint8_t);

void SaveUserParamSet(uint8_t set);
uint8_t RecallUserParamSet(uint8_t set, uint8_t mode);
uint8_t RecallDefaultParamSet(void);
void PushParamSet(void);

void LIMIT_DOUBLE(double *param, double min, double max);
void LIMIT_UINT8(uint8_t *param, uint8_t min, uint8_t max);
void LIMIT_INT16(int16_t *param, int16_t min, int16_t max);
void LIMIT_UINT16(uint16_t *param, uint16_t min, uint16_t max);

// void PrintIDNstring(void);

// dcg-hw.c ///////////////////////////////////////////////
uint16_t GetADC(uint8_t);
#ifdef DUAL_DAC
void ShiftOut1655(uint16_t, uint8_t);
#else
void ShiftOut1655(uint16_t);
#endif
void ShiftOut1257(uint16_t);
uint16_t ShiftIn1864(void);
double GetLM75Temp(void);
void ConfigLM75(double);

// dcg-panel.h ////////////////////////////////////////////
void jobPanel(void);

// dcg-parser.h ///////////////////////////////////////////
void SendTrackCmd(void);
void SendTrackOnOff(void);

#endif
