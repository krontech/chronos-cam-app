#ifndef OVERLAY_H
#define OVERLAY_H

#include "gpmc.h"


#define OVERLAY_BASE    (0x8000)

#define OVERLAY_IDENT                   (OVERLAY_BASE + 0)
#define OVERLAY_VERSION                 (OVERLAY_BASE + 2)
#define OVERLAY_SUBVERSION              (OVERLAY_BASE + 4)
#define OVERLAY_CONTROL                 (OVERLAY_BASE + 6)
#define OVERLAY_CONTROL_TB0_EN_MASK     0x0010  //Textbox 0 enable
#define OVERLAY_CONTROL_TB1_EN_MASK     0x0020  //Textbox 1 enable
#define OVERLAY_CONTROL_WM_EN_MASK      0x0040  //Watermark enable
#define OVERLAY_CONTROL_LM_EN_MASK      0x0080  //Logo enable
#define OVERLAY_CONTROL_FS_OFFSET       8       //Font size offset
#define OVERLAY_CONTROL_FS_MASK         0x1F00  //Font size bit mask
#define OVERLAY_CONTROL_BS_MASK         0x8000  //Bank select (0 = font memory, 1 = logo meory)


#define OVERLAY_STATUS                  (OVERLAY_BASE + 8)
#define OVERLAY_TEXTBOX0_XPOS           (OVERLAY_BASE + 0xA)
#define OVERLAY_TEXTBOX0_YPOS           (OVERLAY_BASE + 0xC)
#define OVERLAY_TEXTBOX0_XSIZE          (OVERLAY_BASE + 0xE)
#define OVERLAY_TEXTBOX0_YSIZE          (OVERLAY_BASE + 0x10)
#define OVERLAY_TEXTBOX1_XPOS           (OVERLAY_BASE + 0x12)
#define OVERLAY_TEXTBOX1_YPOS           (OVERLAY_BASE + 0x14)
#define OVERLAY_TEXTBOX1_XSIZE          (OVERLAY_BASE + 0x16)
#define OVERLAY_TEXTBOX1_YSIZE          (OVERLAY_BASE + 0x18)
#define OVERLAY_WATERMARK_XPOS          (OVERLAY_BASE + 0x1A)
#define OVERLAY_WATERMARK_YPOS          (OVERLAY_BASE + 0x1C)
#define OVERLAY_TEXTBOX01_XOFFSET       (OVERLAY_BASE + 0x1E)
#define OVERLAY_TEXTBOX01_YOFFSET       (OVERLAY_BASE + 0x20)
#define OVERLAY_RGBLOGO_XPOS            (OVERLAY_BASE + 0x22)
#define OVERLAY_RGBLOGO_YPOS            (OVERLAY_BASE + 0x24)
#define OVERLAY_RGBLOGO_XSIZE           (OVERLAY_BASE + 0x26)
#define OVERLAY_RGBLOGO_YSIZE           (OVERLAY_BASE + 0x28)
#define OVERLAY_TEXTBOX0_BLUE_ALPHA     (OVERLAY_BASE + 0x2A)
#define OVERLAY_TEXTBOX0_RED_GREEN      (OVERLAY_BASE + 0x2C)
#define OVERLAY_TEXTBOX1_BLUE_ALPHA     (OVERLAY_BASE + 0x2E)
#define OVERLAY_TEXTBOX1_RED_GREEN      (OVERLAY_BASE + 0x30)
#define OVERLAY_WATERMARK_BLUE_ALPHA    (OVERLAY_BASE + 0x32)
#define OVERLAY_WATERMARK_RED_GREEN     (OVERLAY_BASE + 0x34)

#define OVERLAY_TEXTBOX0_BUFFER         (OVERLAY_BASE + 0x100)
#define OVERLAY_TEXTBOX1_BUFFER         (OVERLAY_BASE + 0x200)
#define OVERLAY_RGBLOGO_R_LUT           (OVERLAY_BASE + 0x300)
#define OVERLAY_RGBLOGO_G_LUT           (OVERLAY_BASE + 0x400)
#define OVERLAY_RGBLOGO_B_LUT           (OVERLAY_BASE + 0x500)
#define OVERLAY_TEXTBOX0_FONT_BITMAP    (OVERLAY_BASE + 0x1000)
#define OVERLAY_TEXTBOX1_FONT_BITMAP    (OVERLAY_BASE + 0x4000)
#define OVERLAY_RGBLOGO_BITMAP          (OVERLAY_BASE + 0x1000)



class Overlay
{
public:
    Overlay();
    ~Overlay();
    CameraErrortype init(GPMC * gpmc_inst);
    CameraErrortype setTextbox0Text(char * str);
    CameraErrortype setTextbox1Text(char * str);
    CameraErrortype setTextbox0Enable(bool en);
    CameraErrortype setTextbox1Enable(bool en);
    CameraErrortype setTextboxPosition(UInt32 tbIndex, UInt16 xPos, UInt16 yPos);
    CameraErrortype setTextboxSize(UInt32 tbIndex, UInt16 xSize, UInt16 ySize);
    GPMC * gpmc;

};


#endif // OVERLAY_H

