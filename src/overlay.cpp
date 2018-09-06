/****************************************************************************
 *  Copyright (C) 2013-2017 Kron Technologies Inc <http://www.krontech.ca>. *
 *                                                                          *
 *  This program is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/
#include <QDebug>
#include "gpmc.h"
#include "util.h"
#include "defines.h"
#include "cameraRegisters.h"

#include "types.h"
#include "overlay.h"


Overlay::Overlay()
{

}

Overlay::~Overlay()
{

}

CameraErrortype Overlay::init(GPMC * gpmc_inst)
{

    gpmc = gpmc_inst;

    gpmc->write16(OVERLAY_CONTROL, 0x1900); //Turn off all overlays


    return SUCCESS;
}

CameraErrortype Overlay::setTextbox0Text(char * str)
{
    int i;

    for(i = 0; i < strlen(str); i++)
    {
        gpmc->write8(OVERLAY_TEXTBOX0_BUFFER + i, str[i]);
    }
    gpmc->write8(0x8100+i,'\0');    //Write the null terminator

    return SUCCESS;
}

CameraErrortype Overlay::setTextbox1Text(char * str)
{
    int i;

    for(i = 0; i < strlen(str); i++)
    {
        gpmc->write8(OVERLAY_TEXTBOX1_BUFFER + i, str[i]);
    }
    gpmc->write8(0x8100+i,'\0');    //Write the null terminator

    return SUCCESS;
}

CameraErrortype Overlay::setTextbox0Enable(bool en)
{
    UInt16 control = gpmc->read16(OVERLAY_CONTROL);
    gpmc->write16(OVERLAY_CONTROL, (control & ~OVERLAY_CONTROL_TB0_EN_MASK) | (en ? OVERLAY_CONTROL_TB0_EN_MASK : 0));

    return SUCCESS;
}

CameraErrortype Overlay::setTextbox1Enable(bool en)
{
    UInt16 control = gpmc->read16(OVERLAY_CONTROL);
    gpmc->write16(OVERLAY_CONTROL, (control & ~OVERLAY_CONTROL_TB1_EN_MASK) | (en ? OVERLAY_CONTROL_TB1_EN_MASK : 0));

    return SUCCESS;
}

CameraErrortype Overlay::setTextboxPosition(UInt32 tbIndex, UInt16 xPos, UInt16 yPos)
{
    UInt32 XAddr = tbIndex == 0 ? OVERLAY_TEXTBOX0_XPOS : OVERLAY_TEXTBOX1_XPOS;
    UInt32 YAddr = tbIndex == 0 ? OVERLAY_TEXTBOX0_YPOS : OVERLAY_TEXTBOX1_YPOS;

    gpmc->write16(XAddr, xPos);
    gpmc->write16(YAddr, yPos);

    return SUCCESS;
}

CameraErrortype Overlay::setTextboxSize(UInt32 tbIndex, UInt16 xSize, UInt16 ySize)
{
    UInt32 XAddr = tbIndex == 0 ? OVERLAY_TEXTBOX0_XSIZE : OVERLAY_TEXTBOX1_XSIZE;
    UInt32 YAddr = tbIndex == 0 ? OVERLAY_TEXTBOX0_YSIZE : OVERLAY_TEXTBOX1_YSIZE;

    gpmc->write16(XAddr, xSize);
    gpmc->write16(YAddr, ySize);

    return SUCCESS;
}



