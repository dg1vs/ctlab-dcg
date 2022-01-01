/*
 * Copyright (c) 2009-2016 by Paul Schmid
 * Copyright (c) 2007, 2008 by Hartmut Birr
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
#include <avr/pgmspace.h>
#include <math.h>

#include "dcg.h"
#include "encoder.h"
#include "Parser.h"
#include "Lcd.h"
#include "config.h"

#define NDEBUG
#include "debug.h"

int16_t EncDiff;
int16_t EncDiffSpeed;

uint8_t Mode;       // Mode = 0: Modification of the set value itself (with ModifyTimer running)
                    // Mode = 0: Display of measured value (in voltage / current mode (without ModifyTimer running)
                    // Mode = 1: Modification of Cursor Position
                    // Mode = 2: Range modification
uint16_t ModifyTimer;
uint8_t DisplayTimer;
uint8_t ModifyPos;
uint8_t ModifyChr;

//Menu for parameter selection
//the length of the table is 1+2*USERSETS
//currently configured for USERSETS=3
const PROGMEM char ccParamSet[][9] =
{
    "Ld Deflt",     //  0
    "Ld Usr 1",     //  1
    "Ld Usr 2",     //  2
    "Ld Usr 3",     //  3
    "St Usr 1",     //  4
    "St Usr 2",     //  5
    "St Usr 3",     //  6
};

//Menu for start parameter set selection
//the length of the table is 1+USERSETS
//currently configured for USERSETS=3
const PROGMEM char ccStartParamSet[][9] =
{
    "   Deflt",     //  0
    "   Usr 1",     //  1
    "   Usr 2",     //  2
    "   Usr 3",     //  3
};


enum MenuOptions
{
    ModifyVolt = 0,
    ModifyAmp,
    ModifyOutput,
    ModifyTrack,

    ModifyRippleMod,
    ModifyRippleOnTime,
    ModifyRippleOffTime,
    ReturnRipple,

    ModifyArbMode,
    ModifyArbSelect,
    ModifyArbRepeat,
    ModifyArbDelay,
    ReturnArb,

    ModifySaveRecall,
    ModifyStartParamSet,
    ShowPower,
    ModifyAmpWattHours
};

uint8_t Modify = ModifyVolt;


#define NONE 0xff
#define MENUENTRIES 17
#define MENULENGTH 4

const PROGMEM uint8_t MenuArray[MENUENTRIES][MENULENGTH] PROGMEM =
{
//                            Menu                 Next                 Previous             Submenu
    /* MenuVolt */          { ModifyVolt,          ModifyAmp,           ModifyAmpWattHours,  NONE               },
    /* MenuAmp */           { ModifyAmp,           ModifyOutput,        ModifyVolt,          NONE               },
    /* MenuOutput */        { ModifyOutput,        ModifyTrack,         ModifyAmp,           NONE               },
    /* MenuTrack */         { ModifyTrack,         ModifyRippleMod,     ModifyOutput,        NONE               },
    /* MenuRippleMod */     { ModifyRippleMod,     ModifyArbMode,       ModifyTrack,         ModifyRippleOnTime },
    /* MenuRippleOnTime */  { ModifyRippleOnTime,  ModifyRippleOffTime, ReturnRipple,        NONE               },
    /* MenuRippleOffTime */ { ModifyRippleOffTime, ReturnRipple,        ModifyRippleOnTime,  NONE               },
    /* MenuReturnRipple */  { ReturnRipple,        ModifyRippleOnTime,  ModifyRippleOffTime, ModifyRippleMod    },
    /* MenuArbMode */       { ModifyArbMode,       ModifySaveRecall,    ModifyRippleMod,     ModifyArbSelect    },
    /* MenuArbSelect */     { ModifyArbSelect,     ModifyArbRepeat,     ReturnArb,           NONE               },
    /* MenuArbRepeat */     { ModifyArbRepeat,     ModifyArbDelay,      ModifyArbSelect,     NONE               },
    /* MenuArbDelay */      { ModifyArbDelay,      ReturnArb,           ModifyArbRepeat,     NONE               },
    /* MenuReturnArb */     { ReturnArb,           ModifyArbSelect,     ModifyArbDelay,      ModifyArbMode      },
    /* MenuSaveRecall */    { ModifySaveRecall,    ModifyStartParamSet, ModifyArbMode,       NONE               },
    /* MenuStartParamSet */ { ModifyStartParamSet, ShowPower,           ModifySaveRecall,    NONE               },
    /* MenuShowPower */     { ShowPower,           ModifyAmpWattHours,  ModifyStartParamSet, NONE               },
    /* MenuAmpWattHours */  { ModifyAmpWattHours,  ModifyVolt,          ShowPower,           NONE               }
};


#define ENTRY 0
#define NEXT 1
#define PREV 2
#define SUB 3

void SetMenu( uint8_t *pMenu, uint8_t Mode)
{
    uint8_t i;
    uint8_t tmp;

    for ( i=0; i < MENUENTRIES ; i++)
    {
        memcpy_P(&tmp, &MenuArray[i][ENTRY], sizeof(uint8_t));
        if ( tmp == *pMenu )
            break; // entry found in menu arry
    }

    if ( tmp != *pMenu )
        return;  // return without change; should not happen

    memcpy_P(&tmp, &MenuArray[i][Mode], sizeof(uint8_t));

    if ( tmp == NONE )
        return;  // return without change in case of no submenu. should not happen with "next" and "previous"

    *pMenu = tmp; // assign new menu value
    return;
}

struct
{
    uint8_t BtnDown         : 1;    ///< state of the DOWN button
    uint8_t BtnUp           : 1;    ///< state of the UP button
    uint8_t BtnEnter        : 1;    ///< state of the ENTER button
    uint8_t lastBtnDown     : 1;    ///< previous state of the DOWN button
    uint8_t lastBtnUp       : 1;    ///< previous state of the UP button
    uint8_t lastBtnEnter    : 1;    ///< previous state of the ENTER button
    uint8_t BtnDownCnt      : 3;    ///< counter which is used to detect if the DOWN button id pressed
    uint8_t BtnUpCnt        : 3;    ///< counter which is used to detect if the UP button id pressed
    uint8_t BtnEnterCnt     : 3;    ///< counter which is used to detect if the ENTER button id pressed
} PanelData;



uint8_t PanelPrintFault(char *line)
{

    if (ModifyTimer)    // don't display fault in case encoder was turned
        return 0;

    if (ToggleDisplay == 0x00)
    {
        if (Status.FuseBlown)
        {
            sprintf_P(line, PSTR("%S"), PSTR("FUSEBLW "));
        }
        else if (Status.OverVolt)
        {
            sprintf_P(line, PSTR("%S"), PSTR("OVRVOLT "));
        }
        else if (Status.OverTemp)
        {
            sprintf_P(line, PSTR("%S"), PSTR("OVRTEMP "));
        }
        else
        {
            return 0;
        }

        return 1;
    }

    return 0;

}

//---------------------------------------------------------------------------------------------
/// This function calculates the position of the visible starting digit and the position of
/// the decimal point of the given current.
///
/// \param  :  value with resolution of e-06
///
/// \return uint8_t : high nibble -> position of decimal point
///                   low nibble  -> position of the first visible digit
//---------------------------------------------------------------------------------------------
uint8_t GetValuePosition(float value, uint8_t mode)
{

    /*  value       "position"  expo    display     value in
                    high|low                        1.0e-6
        < 0.01      0x30        -2      9.999m      000 000.009 999
        < 0.1       0x31        -1      99.99m      000 000.099 999
        < 1.0       0x32         0      999.9m      000 000.999 999
        < 10.0      0x63         1      9.999       000 009.999 999
        < 100.0     0x64         2      99.99       000 099.999 999
        < 1.0k      0x65         3      999.9       000 999.999 999
        < 10.0k     0x96         4      9.999k      009 999.999 999
        < 100.0k    0x97         5      99.99k      099 999.999 999
        < 1.0M      0x98         6      999.9k      999 999.999 999
    */


    if ( (mode == VOLT) || (mode == ModifyVolt) )
    {

        if (value < 9.9995)  //10.0)   // caution! sprintf in FloatToString uses rounding after the last digit!
        {
            return 0x63;
        }
        else
        {
            return 0x64;
        }

    }
    else
    {
        if (value < 0.0099995)  //0.01)
        {
            return 0x30;
        }
        else if (value < 0.099995)  //0.1)
        {
            return 0x31;
        }
        else if (value < 0.99995)  //1.0)
        {
            return 0x32;
        }
        else if (value < 9.9995)  //10.0)
        {
            return 0x63;
        }
        else if (value < 99.995)  //100.0)
        {
            return 0x64;
        }
        else if (value < 999.95)  //1000.0)
        {
            return 0x65;
        }
        else if (value < 9999.5)  //10000.0)
        {
            return 0x96;
        }
        else if (value < 99995)  //100000.0)
        {
            return 0x97;
        }
        else
        {
            return 0x98;
        }

    }

}


//---------------------------------------------------------------------------------------------
// This function calculates the position of the visible starting digit and the position of
/// the decimal point of the given time value for ripple function.
///
/// \param uiTime : time value in ms
///
/// \return uint8_t : high nibble -> position of decimal point
///                   low nibble  -> position of the first visible digit
//---------------------------------------------------------------------------------------------
uint8_t GetTimePosition(uint16_t uiTime)
{
    if (uiTime <=35550) // to satisfy usage of uiTime ****
        // always Milliseconds with a resolution of one Millisecond
        return 0x00;
    return 0x00;
}

//---------------------------------------------------------------------------------------------
/// These functions convert parameter values into a string for the Display
//---------------------------------------------------------------------------------------------

void OutputToString(char* str, uint8_t t)
{
    if (t == 0)
    {
        sprintf_P(str, PSTR("    Off "));
    }
    else
    {
        sprintf_P(str, PSTR("    On  "));
    }
}

void TrackToString(char* str, uint8_t t)
{
    if (t == 255)
    {
        sprintf_P(str, PSTR("    Off "));
    }
    else
    {
        sprintf_P(str, PSTR("Addr: %d "), t);
    }
}

void ArbModeToString(char* str, uint8_t t)
{
    switch (t)
    {
        case 0:
        default:
            sprintf_P(str, PSTR("    Off "));
            break;
        case 1:
            sprintf_P(str, PSTR("    ROM "));
            break;
        case 2:
            sprintf_P(str, PSTR("    RAM "));
            break;
    }
}

void ArbLabelToString(char* str, uint8_t t)
{
    const char* ppstr;


    if (ArbActive != 2)
    {

        // let's hope that t will never be out of range ;-)

        memcpy_P(&ppstr, &ArbArrayL[t], sizeof(const char*));
        sprintf_P(str, ppstr);

    }
    else
    {
        sprintf_P(str, PSTR("RAM #%1d  "), t);
    }

}

void ArbRepeatToString(char* str, uint8_t t)
{
    if (t == 255)
    {
        sprintf_P(str, PSTR("   Cont "));
    }
    else
    {
        sprintf_P(str, PSTR("    %3d "), t);
    }
}

void PStringToString(char* str, const char* ppstr)
{
    sprintf_P(str, ppstr);
}


void Uint16ToString(char* str, uint16_t t, uint8_t mode)
{

    const char * pUnit= 0;

    switch (mode)
    {
        case SECOND:
            pUnit = PSTR("ms");
            break;

        case PERCENT:
            pUnit = PSTR(" %");
            break;

        case BLANK:
            pUnit = PSTR("  ");
            break;
    }
    //**** ganze Zahlen, ohne Komma !?
    sprintf_P(str, PSTR("%5d%S "), t, pUnit);  // capital %s for PSTR
}


void PercentToString(char* str, uint16_t p)
{
    if (p == 0)
    {
        sprintf_P(str, PSTR("    Off "));
    }
    else
    {
        Uint16ToString(str, p, PERCENT);
    }
}

void RangeCurrentToString(char* str, uint8_t Range)
{
    uint8_t i;
    if (Range <= DC2000mA)
    {
        if (Params.MaxCurrent[Range] < 0.001)
        {
            i = sprintf_P(str, PSTR("  %.0f\344A"), Params.MaxCurrent[Range] * 1e6);
        }
        else if (Params.MaxCurrent[Range] < 1.0)
        {
            i = sprintf_P(str, PSTR("  %.0fmA"), Params.MaxCurrent[Range] * 1e3);
        }
        else if (Params.MaxCurrent[Range] < 1000.0)
        {
            i = sprintf_P(str, PSTR("  %.0fA"), Params.MaxCurrent[Range]);
        }
        else
        {
            i = sprintf_P(str, PSTR("  %.0fkA"), Params.MaxCurrent[Range] * 0.001f);
        }
    }
    else
    {
        i = sprintf_P(str, PSTR("  auto"));
    }
    while (i < 8)
    {
        str[i++] = ' ';
    }
    str[8] = 0;
}

void FloatToString(char* str, float i, uint8_t mode)
{
    char cUnit=0;
    char cUnitHour = ' ';
    char FormatString[]= "%4.3f%c%c%c"; // This string is modified, therefore NOT in PSTR
    char cPrefix;
    uint8_t tmp;
    uint8_t highnibble;
    uint8_t lownibble;

    switch (mode)
    {
        case AMP:
            cUnit = 'A';
            break;
        case AMPHOUR:
            cUnit = 'A';
            cUnitHour = 'h';
            break;

        case WATT:
            cUnit = 'W';
            break;
        case WATTHOUR:
            cUnit = 'W';
            cUnitHour = 'h';
            break;

        case VOLT:
            cUnit = 'V';
            break;
    }

    if (i < 0.0)
    {
        i = 0.0;
    }


    tmp = GetValuePosition(i, mode);


    lownibble = tmp % 16;
    highnibble = tmp / 16;

    lownibble = highnibble - lownibble;

    if (highnibble == 3)
        cPrefix = 'm';
    else if (highnibble == 6)
        cPrefix = ' ';
    else // if (highnibble == 9)
        cPrefix = 'k';


    i *= pow(10,6-highnibble);

    FormatString[3] = lownibble + 0x30;
    sprintf(str, FormatString, i , cPrefix ,cUnit, cUnitHour);

}



void RangeVoltageToString(char* str, uint8_t Range)
{
    uint8_t i;
    if (Range <= 1)
    {
        i = sprintf_P(str, PSTR("  %.1fV"), Params.MaxVoltage[Range]);
    }
    else
    {
        i = sprintf_P(str, PSTR("  auto"));
    }
    while (i < 8)
    {
        str[i++] = ' ';
    }
    str[8] = 0;
}


/*
void TemperatureToString(char* str, float i)
{
                                                    // Temperature string in °Celsius
    sprintf_P(str, PSTR("%+5.1f\337C "), i);        // '°' = 0337 =0xdf

}
*/


void PanelPrintU(uint8_t cursor)
{
    char line[9];
    float* dptr;

    dptr = &xVoltage;

    if (RippleActive)
    {
        if (ToggleDisplay)
        {
            dptr = &xVoltageLow;
        }
    }

    FloatToString(line, *dptr, VOLT);

    if (cursor != ' ')
    {
        line[7] = cursor;
    }
    else if (RippleActive)
    {
        line[7] = (ToggleDisplay ? SMALL_R_LOW : SMALL_R_HIGH);
    }
    else if (ArbActive)
    {
        line[7] = (ToggleDisplay ? SMALL_A_INV : SMALL_A );
    }

    Lcd_Write(0, 0, 8, line);
}


void PanelPrintI(uint8_t cursor)
{
    char line[9];
    float* dptr;

    if (!PanelPrintFault(line))
    {
        dptr = &xCurrent;

        if (RippleActive)
        {
            if (ToggleDisplay)
            {
                dptr = &xCurrentLow;
            }
        }

        FloatToString(line, *dptr, AMP );

        if (cursor != ' ')
        {
            line[7] = cursor;
        }
        else if (RippleActive)
        {
            line[7] = (ToggleDisplay ? SMALL_R_LOW : SMALL_R_HIGH);
        }
        else if (ArbActive)
        {
            line[7] = (ToggleDisplay ? SMALL_A_INV : SMALL_A );
        }
    }
    Lcd_Write(0, 1, 8, line);
}


float GetIncrement(int8_t pos)
{
    return pow(10, pos - 6);
}

uint16_t GetTimeIncrement(int8_t pos)
{
    return (uint16_t)(pow(10, pos) + 0.5);
}



void jobPanel(void)
{
    if (Flags.LCDPresent)
    {
        uint8_t ButtonPressed = 0;
        uint8_t IncValid = 0;
        uint8_t Update = 0;
        uint8_t tmp;
        static char DisplayStr[9];
        static int8_t ModifyVoltagePos = -1;
        static int8_t ModifyCurrentPos = -1;
        static uint16_t BtnEnterCount = 0;
        static uint8_t Range;
        uint8_t lastModify = Modify;
        uint8_t lastMode = Mode;
        uint16_t uiIncrement;
        static uint8_t SuppressModifyMode = 0;

        static int8_t ModifyRippleOnPos = -1;
        static int8_t ModifyRippleOffPos = -1;
        static int8_t ModifyArbDelayPos = -1;

        static uint8_t SaveRecall = 0;
        static uint8_t SaveRecallState = 0;
        static uint8_t lastActiveParamSet = 0;

        static int16_t *pModifyParam;
        static int8_t  *pModifyParamPos;
        static uint8_t  *pModifyParam8;
        static const char *PDisplayString;
        uint8_t OutSubCh=0;

        float *pwValue = &wCurrent;
        float RoundFactor = 1.0;
        int8_t *pModifyValuePos = &ModifyCurrentPos;

        static uint8_t GoSub = 0;

        if (ModifyTimer)
        {
            ModifyTimer--;
        }

        if (DisplayTimer)
        {
            DisplayTimer--;
        }

        // save button state
        PanelData.lastBtnDown = PanelData.BtnDown;
        PanelData.lastBtnUp = PanelData.BtnUp;
        PanelData.lastBtnEnter = PanelData.BtnEnter;

        tmp = Lcd_GetButton();

        // button down
        if (tmp & BUTTON_DOWN)
        {
            if (PanelData.BtnDownCnt < 1)
            {
                PanelData.BtnDownCnt++;
            }
        }
        else
        {
            if (PanelData.BtnDownCnt > 0)
            {
                PanelData.BtnDownCnt--;
            }
        }
        if (PanelData.BtnDown)
        {
            if (PanelData.BtnDownCnt == 0)
            {
                PanelData.BtnDown = 0;
            }
        }
        else
        {
            if (PanelData.BtnDownCnt == 1)
            {
                PanelData.BtnDown = 1;
            }
        }

        // button up
        if (tmp & BUTTON_UP)
        {
            if (PanelData.BtnUpCnt < 1)
            {
                PanelData.BtnUpCnt++;
            }
        }
        else
        {
            if (PanelData.BtnUpCnt > 0)
            {
                PanelData.BtnUpCnt--;
            }
        }
        if (PanelData.BtnUp)
        {
            if (PanelData.BtnUpCnt == 0)
            {
                PanelData.BtnUp = 0;
            }
        }
        else
        {
            if (PanelData.BtnUpCnt == 1)
            {
                PanelData.BtnUp = 1;
            }
        }

        // button enter
        if (tmp & BUTTON_ENTER)
        {
            if (PanelData.BtnEnterCnt < 1)
            {
                PanelData.BtnEnterCnt++;
            }
        }
        else
        {
            if (PanelData.BtnEnterCnt > 0)
            {
                PanelData.BtnEnterCnt--;
            }
        }
        if (PanelData.BtnEnter)
        {
            if (PanelData.BtnEnterCnt == 0)
            {
                PanelData.BtnEnter = 0;
            }
        }
        else
        {
            if (PanelData.BtnEnterCnt == 1)
            {
                PanelData.BtnEnter = 1;
            }
        }

        if (PanelData.BtnDown && !PanelData.lastBtnDown)
        {
            DPRINT(PSTR("Down button pressed\n"));
            ButtonPressed = 1;

            SetMenu(&Modify, NEXT);

            Mode = 0;
            SerPrompt(NoErr, 65);
        }

        if(PanelData.BtnUp && !PanelData.lastBtnUp)
        {
            DPRINT(PSTR("Up button pressed\n"));
            ButtonPressed = 1;

            SetMenu(&Modify, PREV);

            Mode = 0;
            SerPrompt(NoErr, 66);
        }


        if (GoSub)  // EnterButton is used as sub menu trigger, depending on currently active menu
        {
            SetMenu(&Modify, SUB);
            Mode = 0;
            SerPrompt(NoErr, 66);
            GoSub = 0;
        }


        if (PanelData.BtnEnter)
        {
            if(!PanelData.lastBtnEnter)
            {
                DPRINT(PSTR("Enter button pressed\n"));
                ButtonPressed = 1;
                if (Mode == 2)
                {
                    Mode = 0;
                }
                else
                {
                    Mode = Mode ? 0 : 1;
                }
                SerPrompt(NoErr, 67);
            }
            if (BtnEnterCount <= 62)
            {
                BtnEnterCount++;
                ModifyTimer = 62;
            }
            if (BtnEnterCount == 62)
            {
                ButtonPressed = 1;
                Mode = 2;
            }
        }
        else
        {
            BtnEnterCount = 0;
            GoSub = 0;
        }

        if(SuppressModifyMode)
        {
            ModifyTimer = 0;
            lastModify = Modify;
            lastMode = Mode;
            ButtonPressed = 0;
            
            if (!PanelData.BtnEnter)
            {
                SuppressModifyMode = 0;
            }
        }

        EncDiffSpeed = EncDiff += Encoder_GetAndResetPosition();

        if (EncDiff != 0)
        {
            IncValid = 1;

            if (Mode == 0)
            {
                if (EncDiff > 2 || EncDiff < -2)
                {
                    EncDiffSpeed = EncDiff * 5;
                }
                else if (EncDiff > 1 || EncDiff < -1)
                {
                    EncDiffSpeed = EncDiff * 2;
                }
            }
        }


        if (Mode != lastMode ||
                Modify != lastModify ||
                ButtonPressed ||
                IncValid ||
                (Mode > 0 && ModifyTimer == 0))
        {
            uint8_t StartPos;
            uint8_t PointPos;

            if ((Mode != lastMode && lastMode == 2) ||
                    (ModifyTimer == 0 && lastMode == 2))
            {
                if (lastModify == ModifyVolt)
                {
                    LockRangeU = Range;
                }
                if (lastModify == ModifyAmp)
                {
                    LockRangeI = Range;
                }

                CheckLimits();
                SetLevelDAC();

                SendTrackCmd();
            }


            if (ModifyTimer == 0 && Mode > 0)
            {
                Mode = 0;
            }

            SetActivityTimer(75);
            ModifyTimer = 62;
            Status.Busy = 1;
            Update = 1;

            switch (Modify)
            {

                uint8_t unit;

                // *** Modify Voltage & Amp **************************************************

                case ModifyVolt:
                case ModifyAmp:


                    if (Modify == ModifyVolt)
                    {
                        pwValue = &wVoltage;
                        pModifyValuePos = &ModifyVoltagePos;
                        RoundFactor = 1e3; // resolution mV for Voltage
                        OutSubCh = 0;
                        unit = VOLT;
                    }
                    else if (Modify == ModifyAmp)
                    {
                        pwValue = &wCurrent;
                        pModifyValuePos = &ModifyCurrentPos;
                        RoundFactor = 1e6;  // resolution µA for Current
                        OutSubCh = 1;
                        unit = AMP;
                    }


                    if (Mode == 2)
                    {
                        if (Modify == ModifyVolt)
                        {
                            if (BtnEnterCount == 62)
                            {
                                Range = LockRangeU;
                            }
                            if (IncValid)
                            {
                                if (EncDiff > 0)
                                {
                                    Range++;
                                }
                                else
                                {
                                    Range--;
                                }
                                EncDiff = 0;
                                if (Range == 254)
                                {
                                    Range = 1;
                                }
                                else if (Range > 1)
                                {
                                    Range = 255;
                                }
                            }
                            RangeVoltageToString(DisplayStr, Range);
                        }
                        else
                        {

                            if ( Modify == ModifyAmp )
                            {

                                if (BtnEnterCount == 62)
                                {
                                    Range = LockRangeI;
                                }
                                if (IncValid)
                                {
                                    if (EncDiff > 0)
                                    {
                                        Range++;
                                    }
                                    else
                                    {
                                        Range--;
                                    }
                                    EncDiff = 0;
                                    if (Range == 254)
                                    {
                                        Range = DC2000mA;
                                    }
                                    else if (Range > DC2000mA)
                                    {
                                        Range = 255;
                                    }
                                }

                                RangeCurrentToString(DisplayStr, Range);
                            }
                            else
                            {
                                EncDiff = 0;
                                RangeCurrentToString(DisplayStr, RangeI);
                            }

                        }

                    }
                    else
                    {
                        tmp = GetValuePosition(*pwValue, Modify);
                        StartPos = tmp % 16;
                        PointPos = tmp / 16;

                        if (*pModifyValuePos == -1)             // only after reset
                        {
                            *pModifyValuePos = StartPos + 2;    // make 2nd digit from left side the default modify digit

                            if (*pModifyValuePos > 5)
                            {
                                *pModifyValuePos = 5;           // maximal initial increment 0.1V / 0.1A
                            }

                        }

                        if (*pModifyValuePos < StartPos)
                        {
                            *pModifyValuePos = StartPos;
                        }
                        if (IncValid)
                        {
                            if (Mode == 1)
                            {
                                *pModifyValuePos -= EncDiff;
                                if (*pModifyValuePos < StartPos)
                                {
                                    *pModifyValuePos = StartPos;
                                }
                                else if (*pModifyValuePos > StartPos + 3)
                                {
                                    *pModifyValuePos = StartPos + 3;
                                }
                            }
                            else
                            {
                                //Calculate new value
                                *pwValue += GetIncrement(*pModifyValuePos) * EncDiff;
                                *pwValue = floor(*pwValue * RoundFactor + 0.5) / RoundFactor;

                                if ( Modify == ModifyVolt )
                                {
                                    DCVoltMod = 1;
                                }

                                if ( Modify == ModifyAmp )
                                {
                                    DCAmpMod = 1;
                                }

                                CheckLimits();
                                SetLevelDAC();

                                SendTrackCmd();

                                ParseGetParam(OutSubCh);

                                tmp = GetValuePosition(*pwValue, Modify);

                                StartPos = tmp % 16;
                                PointPos = tmp / 16;
                                if (*pModifyValuePos < StartPos)
                                {
                                    *pModifyValuePos = StartPos;
                                }
                            }
                            EncDiff = 0;
                        }
                        ModifyPos = 4 - (*pModifyValuePos - StartPos);
                        if (*pModifyValuePos >= PointPos)
                        {
                            ModifyPos--;
                        }

                        FloatToString(DisplayStr, *pwValue, unit);
                        ModifyChr = DisplayStr[ModifyPos];
                    }
                    break;

                //*** ModifyOutput ***********************************************************

                case ModifyOutput:
                {

                    if (Mode == 1)
                        {
                            if (Params.OutputOnOff == 0)
                            {
                                Params.OutputOnOff = 1;
                            }
                            else
                            {
                                Params.OutputOnOff = 0;
                            }
                        
                            // jump back to default screen
                          
                            Modify = ModifyAmp;
                            Update = 0;
                            SuppressModifyMode = 1;
                            Mode = 0;

                            SendTrackOnOff();

                            break;
                    }   
                    else
                    {
                        if (IncValid)
                        {
                            // Output 0=Off , 1..255=On

                            if (EncDiff > 0)
                            {
                                 Params.OutputOnOff = 1;
                            }
                            else
                            {
                                 Params.OutputOnOff = 0;
                            }

                            EncDiff = 0;
                       }

                       SendTrackOnOff();

                    }                    

                    OutputToString(DisplayStr, Params.OutputOnOff);

                }
                break;

                //*** ModifyTrack ***********************************************************

                case ModifyTrack:
                {
                    if (IncValid)
                    {
                        // Channel 255=Off , 0..7

                        if (EncDiff > 0)
                        {
                            TrackCh++;
                        }
                        else
                        {
                            if (TrackCh != 255 )
                                TrackCh--;
                        }

                        EncDiff = 0;

                        if (TrackCh == 8)
                        {
                            TrackCh = 7;
                        }
                    }

                    TrackToString(DisplayStr, TrackCh);

                }
                break;

                // *** ModifyRipplePercent **************************************************

                case ModifyRippleMod:
                {


                    if (Mode == 1)
                    {
                        GoSub = 1;
                    }
                    else

                        if (IncValid)
                        {
                            // Ripple Percent 0..100

                            if (EncDiff > 0)
                            {
                                if (Params.RippleMod < 100 - EncDiffSpeed)
                                    Params.RippleMod += EncDiffSpeed;
                                else
                                    Params.RippleMod = 100;
                            }
                            else
                            {
                                if (Params.RippleMod > 0-EncDiffSpeed)
                                    Params.RippleMod += EncDiffSpeed;
                                else
                                    Params.RippleMod = 0;
                            }

                            CheckLimits();
                            ParseGetParam(29);
                            SetLevelDAC();
                            EncDiff = 0;
                        }

                    PercentToString(DisplayStr, Params.RippleMod);
                }
                break;

                // *** ReturnXXX **************************************************

                case ReturnRipple:
                case ReturnArb:
                {
                    if (Mode == 1)
                    {
                        GoSub = 1;
                    }
                }
                break;

                // *** ModifyRipple___Time **************************************************

                case ModifyRippleOnTime:
                case ModifyRippleOffTime:
                case ModifyArbDelay:

                    if (Modify == ModifyRippleOnTime)
                    {
                        pModifyParam = &Params.RippleOn;
                        pModifyParamPos = &ModifyRippleOnPos;
                        OutSubCh = 27;
                    }
                    else  if (Modify == ModifyRippleOffTime)
                    {
                        pModifyParam = &Params.RippleOff;
                        pModifyParamPos = &ModifyRippleOffPos;
                        OutSubCh = 28;
                    }
                    else // if (Modify == ModifyArbDelay)
                    {
                        pModifyParam = &ArbDelay;
                        pModifyParamPos = &ModifyArbDelayPos;
                        OutSubCh = 185;
                    }

                    if (Mode == 2)
                    {
                        // no range setting for this option
                        Mode = 1 ;
                    }

                    tmp = GetTimePosition(*pModifyParam);
                    StartPos = tmp % 16;
                    PointPos = tmp / 16;
                    if (*pModifyParamPos < StartPos)
                    {
                        *pModifyParamPos = StartPos;
                    }
                    else if (*pModifyParamPos > StartPos + 3)
                    {
                        *pModifyParamPos = StartPos + 3;
                    }

                    if (IncValid)
                    {

                        if (Mode == 1)
                        {
                            *pModifyParamPos -= EncDiff;
                            if (*pModifyParamPos < StartPos)
                            {
                                *pModifyParamPos = StartPos;
                            }
                            else if (*pModifyParamPos > StartPos + 3)
                            {
                                *pModifyParamPos = StartPos + 3;
                            }
                        }
                        else
                        {
                            // Ripple On Time 0..30000 ms

                            uiIncrement = GetTimeIncrement(*pModifyParamPos);

                            if (EncDiff > 0)
                            {
                                if (*pModifyParam < 30000)
                                {
#ifndef DUAL_DAC
                                    if ( 1 == uiIncrement )
                                    {
                                        *pModifyParam += 2 * EncDiffSpeed;
                                    }
                                    else
                                    {
                                        *pModifyParam += uiIncrement * EncDiffSpeed;
                                    }
#else
                                    *pModifyParam += uiIncrement * EncDiffSpeed;
#endif
                                }
                            }
                            else
                            {
                                if (*pModifyParam > 0)
                                {
#ifndef DUAL_DAC
                                    if ( 1 == uiIncrement)
                                    {
                                        *pModifyParam += 2 * EncDiffSpeed;
                                    }
                                    else
                                    {
                                        *pModifyParam += uiIncrement * EncDiffSpeed;
                                    }
#else
                                    *pModifyParam += uiIncrement * EncDiffSpeed;
#endif
                                }
                            }

                            CheckLimits();
                            ParseGetParam(OutSubCh);
                            SetLevelDAC();

                            tmp = GetTimePosition(*pModifyParam);
                            StartPos = tmp % 16;
                            PointPos = tmp / 16;
                            if (*pModifyParamPos < StartPos)
                            {
                                *pModifyParamPos = StartPos;
                            }
                            else if (*pModifyParamPos > StartPos + 3)
                            {
                                *pModifyParamPos = StartPos + 3;
                            }
                        }

                        EncDiff = 0;
                    }

                    ModifyPos = 5 - (*pModifyParamPos - StartPos);
                    if (*pModifyParamPos >= PointPos)
                    {
                        ModifyPos--;
                    }

                    Uint16ToString(DisplayStr, *pModifyParam, SECOND);
                    ModifyChr = DisplayStr[ModifyPos];

                    break;



                // *** ModifyArbXXX **************************************************

                case ModifyArbMode:
                case ModifyArbSelect:
                case ModifyArbRepeat:

                {

                    if (Modify == ModifyArbMode)
                    {
                        if (Mode == 1)
                        {
                            GoSub = 1;
                        }
                        pModifyParam8 = &ArbActive;
                    }
                    else if (Modify == ModifyArbSelect)
                    {
                        if (ArbActive == 2)

                            pModifyParam8 = &ArbSelectRAM;  // RAM mode
                        else
                            pModifyParam8 = &ArbSelect;  // ROM mode
                    }
                    else // if (Modify == ModifyArbRepeat)
                    {
                        if (Mode == 1)
                        {
                            SetLevelDAC();  // To trigger repetition of the arbitrary signal
                            Mode = 0;
                        }
                        pModifyParam8 = &ArbRepeat;
                    }





                    if (IncValid && ( Mode == 0 ))
                    {
                        if (EncDiff > 0)
                        {
                            if (*pModifyParam8 < 255 - EncDiffSpeed)
                                *pModifyParam8 += EncDiffSpeed;
                            else
                                *pModifyParam8 = 255;
                        }
                        else
                        {
                            if (*pModifyParam8 > 0-EncDiffSpeed)
                                *pModifyParam8 += EncDiffSpeed;
                            else
                                *pModifyParam8 = 0;
                        }

                        CheckLimits();
                        SetLevelDAC();
                        EncDiff = 0;
                    }

                    if (Modify == ModifyArbMode)
                    {
                        ArbModeToString(DisplayStr, ArbActive);
                    }
                    else if (Modify == ModifyArbSelect)
                    {
                        ArbLabelToString(DisplayStr, *pModifyParam8);
                    }
                    else if (Modify == ModifyArbRepeat)
                    {
                        ArbRepeatToString(DisplayStr, ArbRepeat);
                    }
                }
                break;


                // *** ModifySaveRecall **************************************************

                case ModifySaveRecall:
                {
                    static uint8_t lastActiveParamSet = 0;

                    if ((lastModify != ModifySaveRecall) || (lastActiveParamSet != ActiveParamSet))
                    {
                        SaveRecall = lastActiveParamSet = ActiveParamSet;
                    }

                    if (SaveRecallState != 0) // next action after "Done" or "Empty" will revert to normal operation
                    {
                        Mode = 0;
                        SaveRecallState = 0;
                        break;
                    }

                    if (Mode == 1)
                    {

                        ActiveParamSet = (SaveRecall <= USERSETS) ? SaveRecall: SaveRecall-USERSETS ;

                        if (SaveRecall > USERSETS)
                        {
                            SaveUserParamSet(ActiveParamSet - 1);
                            SaveRecallState = 1; // Done
                        }
                        else if (SaveRecall == 0)
                        {
                            if ( 0 == RecallDefaultParamSet() )
                            {
                                CheckLimits();
                                SetLevelDAC();
                                SendTrackCmd();
                                PushParamSet();
                                SaveRecallState = 1; // Done
                            }
                            else
                            {
                                SaveRecallState = 2; // Empty
                            }
                        }
                        else if ((SaveRecall > 0) || (SaveRecall <= USERSETS))
                        {

                            if ((ActiveParamSet - 1) == RecallUserParamSet(ActiveParamSet - 1, RECALL))
                            {
                                CheckLimits();
                                SetLevelDAC();
                                SendTrackCmd();
                                PushParamSet();
                                SaveRecallState = 1; // Done
                            }
                            else
                                SaveRecallState = 2; // Empty
                        }
                        EncDiff = 0;
                    }
                    else
                    {
                        SaveRecallState = 0;
                        if (IncValid)
                        {
                            // 0..6 = Load Default, Recall User 1, Recall User 2, Recall User 3, Save User 1, Save User 2, Save User 3

                            if (EncDiff > 0)
                            {
                                if (SaveRecall < (2*USERSETS))
                                {
                                    SaveRecall++;
                                }
                            }
                            else
                            {
                                if (SaveRecall > 0)
                                {
                                    SaveRecall--;
                                }
                            }

                            EncDiff = 0;

                        }

                    }
                }
                break;

                // *** ModifyStartParamSet **************************************************

                case ModifyStartParamSet:
                {

                    if (lastModify != ModifyStartParamSet)
                    {
                        wStartParamSet = eeprom_read_byte(&StartParamSet);
                    }

                    if (Mode == 1)
                    {

                        if ( (wStartParamSet == 0) || ( (wStartParamSet - 1) == RecallUserParamSet(wStartParamSet-1, CHECK)))
                        {
                            eeprom_write_byte(&StartParamSet, wStartParamSet);
                            SaveRecallState = 1; // Done
                            EncDiff = 0;
                        }
                        else
                        {
                            SaveRecallState = 2; // Empty
                            EncDiff = 0;
                        }
                    }
                    else
                    {
                        SaveRecallState = 0;
                        if (IncValid)
                        {
                            // 0..3 = Default, User 1, User 2,

                            if (EncDiff > 0)
                            {
                                if (wStartParamSet < 3)
                                {
                                    wStartParamSet++;
                                }
                            }
                            else
                            {
                                if (wStartParamSet > 0)
                                {
                                    wStartParamSet--;
                                }
                            }

                            EncDiff = 0;

                        }
                    }
                }
                break;

                // *** Show Power *************************************************************************

                case ShowPower:

                    if (IncValid)
                    {
                        EncDiff = 0;    // Invalidate any encoder entry and clear
                        Update = 0;     // Avoid update of display with encoder ticks
                    }

                    break;

                // *** ShowAmpWattHours **************************************************

                case ModifyAmpWattHours:

                    if (IncValid)
                    {
                        EncDiff = 0;    // Invalidate any encoder entry and clear
                        Update = 0;     // Avoid update of display with encoder ticks
                    }

                    if (Mode == 1)
                    {
                        xAmpHours = 0.0f;
                        xWattHours = 0.0f;
                    }

                    break;

            }
        }

        if (DisplayTimer == 0 || DisplayTimer == 8)
        {
            Update = 1;
            if (DisplayTimer == 0)
            {
                DisplayTimer = 16;
            }
        }

        if (ModifyTimer == 0)
        {
            Mode = 0;
            Status.Busy = 0;
        }

        if (Update)
        {
            switch(Modify)
            {

                // *** Display Voltage **************************************************

                case ModifyVolt:
                    if (Mode == 2)
                    {
                        Lcd_Write_P(0, 0, 8, PSTR("Range U "));
                        Lcd_Write(0, 1, 8, DisplayStr);
                    }
                    else
                    {
                        PanelPrintI(' ');
                        if (ModifyTimer)
                        {
                            if (ModifyPos <= 4)
                            {
                                if (DisplayTimer > 8)
                                {
                                    DisplayStr[ModifyPos] = Mode ? BLOCK_HATCHED : BLOCK_SOLID;
                                }
                                else
                                {
                                    DisplayStr[ModifyPos] = ModifyChr;
                                }
                            }
                            DisplayStr[7] = Mode ? CURSOR_HATCHED : CURSOR_SOLID;
                            Lcd_Write(0, 0, 8, DisplayStr);
                        }
                        else
                        {
                            PanelPrintU(CURSOR_CONCAVE);
                        }
                    }
                    break;

                // *** Display Amp **************************************************

                case  ModifyAmp:
                    if (Mode == 2)
                    {
                        Lcd_Write_P(0, 0, 8, PSTR("Range I "));
                        Lcd_Write(0, 1, 8, DisplayStr);
                    }
                    else
                    {
                        PanelPrintU(' ');

                        if (ModifyTimer)
                        {
                            if (ModifyPos <= 4)
                            {
                                if (DisplayTimer > 8)
                                {
                                    DisplayStr[ModifyPos] = Mode ? BLOCK_HATCHED : BLOCK_SOLID;
                                }
                                else
                                {
                                    DisplayStr[ModifyPos] = ModifyChr;
                                }
                            }
                            DisplayStr[7] = Mode ? CURSOR_HATCHED : CURSOR_SOLID;
                            Lcd_Write(0, 1, 8, DisplayStr);
                        }
                        else
                        {
                            PanelPrintI(CURSOR_CONCAVE);
                        }
                    }
                    break;

                // *** ModifyOutput **************************************************

                case ModifyOutput:

                if (Mode == 2)
                {
                    // no range setting for this option
                    Mode = 1 ;
                }
                else
                {
                    Lcd_Write_P(0, 0, 8, PSTR("Output  "));

                    if (!ModifyTimer)
                    {
                        if (!PanelPrintFault(DisplayStr))
                        {
                            OutputToString(DisplayStr, Params.OutputOnOff);
                        }
                    }
                    Lcd_Write(0, 1, 8, DisplayStr);
                }
                break;


                // *** ModifyTrack **************************************************

                case ModifyTrack:

                    if (Mode == 2)
                    {
                        // no range setting for this option
                        Mode = 1 ;
                    }
                    else
                    {
                        Lcd_Write_P(0, 0, 8, PSTR("Tracking"));

                        if (ModifyTimer)
                        {
                            DisplayStr[7] = CURSOR_SOLID;
                        }
                        else
                        {
                            if (!PanelPrintFault(DisplayStr))
                            {
                                TrackToString(DisplayStr, TrackCh);
                                DisplayStr[7] = CURSOR_CONCAVE;
                            }
                        }
                        Lcd_Write(0, 1, 8, DisplayStr);
                    }
                    break;


                // *** ModifyRippleMod **************************************************

                case ModifyRippleMod:

                    if (Mode == 2)
                    {
                        // no range setting for this option
                        Mode = 1 ;
                    }
                    else
                    {
                        Lcd_Write_P(0, 0, 8, PSTR("Ripple ~"));  // "~" = Arrow to the right to indicate the available submenu


                        if (ModifyTimer)
                        {
                            DisplayStr[7] = CURSOR_SOLID;
                        }
                        else
                        {
                            if (!PanelPrintFault(DisplayStr))
                            {
                                PercentToString(DisplayStr, Params.RippleMod);
                                DisplayStr[7] = CURSOR_CONCAVE;
                            }
                        }
                        Lcd_Write(0, 1, 8, DisplayStr);
                    }
                    break;

                // *** ReturnXXX **************************************************

                case ReturnRipple:
                case ReturnArb:
                {
                    Lcd_Write_P(0, 0, 8, PSTR("  back ~"));  // "~" = Arrow to the right to indicate the available submenu
                    Lcd_Write_P(0, 1, 8, PSTR("        "));
                }
                break;

                // *** Display Ripple___Time **************************************************

                case ModifyRippleOnTime:
                case ModifyRippleOffTime:
                case ModifyArbDelay:

                    if (Modify == ModifyRippleOnTime)
                    {
                        pModifyParam = &Params.RippleOn;
                        PDisplayString = PSTR("RippleOn");
                    }
                    else if (Modify == ModifyRippleOffTime)
                    {
                        pModifyParam = &Params.RippleOff;
                        PDisplayString = PSTR("RippleOff");
                    }
                    else // if (Modify == ModifyArbDelay)
                    {
                        pModifyParam = &ArbDelay;
                        PDisplayString = PSTR("ArbDelay");
                    }

                    if (Mode == 2)
                    {
                        // no range setting for this option
                        Mode = 1 ;
                    }
                    else
                    {
                        Lcd_Write_P(0, 0, 8, PDisplayString);
                        if (ModifyTimer)
                        {
                            if (ModifyPos <= 4)
                            {
                                if (DisplayTimer > 8)
                                {
                                    DisplayStr[ModifyPos] = Mode ? BLOCK_HATCHED : BLOCK_SOLID;
                                }
                                else
                                {
                                    DisplayStr[ModifyPos] = ModifyChr;
                                }
                            }
                            DisplayStr[7] = Mode ? CURSOR_HATCHED : CURSOR_SOLID;
                        }
                        else
                        {
                            if (!PanelPrintFault(DisplayStr))
                            {
                                Uint16ToString(DisplayStr, *pModifyParam, SECOND);
                                DisplayStr[7] = CURSOR_CONCAVE;
                            }
                        }
                        Lcd_Write(0, 1, 8, DisplayStr);
                    }
                    break;


                // *** ModifyArbXXX **************************************************

                case ModifyArbMode:
                case ModifyArbSelect:
                case ModifyArbRepeat:


                    if (Modify == ModifyArbMode)
                    {
                        PDisplayString = PSTR("ArbMode~"); // "~" = Arrow to the right to indicate the available submenu
                    }
                    else if (Modify == ModifyArbSelect)
                    {
                        PDisplayString = PSTR("ArbSelct");
                    }
                    else // if (Modify == ModifyArbRepeat)
                    {
                        PDisplayString = PSTR("ArbRepea");
                    }


                    if (Mode == 2)
                    {
                        // no range setting for this option
                        Mode = 1 ;
                    }
                    else
                    {
                        Lcd_Write_P(0, 0, 8, PDisplayString);


                        if (ModifyTimer)
                        {
                            DisplayStr[7] = CURSOR_SOLID;
                        }
                        else
                        {
                            if (!PanelPrintFault(DisplayStr))
                            {
                                if (Modify == ModifyArbMode)
                                {
                                    ArbModeToString(DisplayStr, ArbActive);
                                }
                                else if (Modify == ModifyArbSelect)
                                {
                                    if (ArbActive == 2)

                                        ArbLabelToString(DisplayStr, ArbSelectRAM);  // RAM mode
                                    else
                                        ArbLabelToString(DisplayStr, ArbSelect);  // ROM mode
                                }
                                else if (Modify == ModifyArbRepeat)
                                {
                                    ArbRepeatToString(DisplayStr, ArbRepeat);
                                }

                                DisplayStr[7] = CURSOR_CONCAVE;
                            }
                        }
                        Lcd_Write(0, 1, 8, DisplayStr);
                    }
                    break;

                // *** ModifySaveRecall **************************************************

                case ModifySaveRecall:

                    if (lastActiveParamSet != ActiveParamSet) // in case the setting has been changed remotely...
                    {
                        SaveRecall = lastActiveParamSet = ActiveParamSet;
                    }

                    Lcd_Write_P(0, 0, 8, PSTR("ParamSet"));

                    if (ModifyTimer && (Mode == 1))
                    {
                        if (SaveRecallState == 1)
                            Lcd_Write_P(0, 1, 8, PSTR("  Done  "));
                        else if (SaveRecallState == 2)
                            Lcd_Write_P(0, 1, 8, PSTR("  Empty "));
                    }
                    else
                    {
                        if (!PanelPrintFault(DisplayStr))
                        {
                            PStringToString(DisplayStr, ccParamSet[SaveRecall]);
                        }
                        Lcd_Write(0, 1, 8, DisplayStr);
                    }

                    break;

                // *** ModifyStartParamSet **************************************************

                case ModifyStartParamSet:
                    Lcd_Write_P(0, 0, 8, PSTR("OnReset "));

                    if (ModifyTimer && (Mode == 1))
                    {
                        if (SaveRecallState == 1)
                            Lcd_Write_P(0, 1, 8, PSTR("  Done  "));
                        else if (SaveRecallState == 2)
                            Lcd_Write_P(0, 1, 8, PSTR("  Empty "));
                    }
                    else
                    {
                        if (!PanelPrintFault(DisplayStr))
                        {
                            PStringToString(DisplayStr, ccStartParamSet[wStartParamSet]);
                        }
                        Lcd_Write(0, 1, 8, DisplayStr);
                    }

                    break;


                // *** Display Power **************************************************

                case ShowPower:
                    Lcd_Write_P(0, 0, 8, PSTR("Power   "));

                    if (!PanelPrintFault(DisplayStr))
                    {
                        FloatToString(DisplayStr, xPowerTot, WATT);
                    }
                    Lcd_Write(0, 1, 8, DisplayStr);
                    break;


                // *** Display WattHours and AmpHours **************************************************

                case ModifyAmpWattHours:
                    FloatToString(DisplayStr, xAmpHours, AMPHOUR);
                    Lcd_Write(0, 0, 8, DisplayStr);

                    if (!PanelPrintFault(DisplayStr))
                    {
                        FloatToString(DisplayStr, xWattHours, WATTHOUR);
                    }
                    Lcd_Write(0, 1, 8, DisplayStr);
                    break;

            }

        }
    }
}
