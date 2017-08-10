/*
 *  Copyright (c) 2010-2011, Texas Instruments Incorporated
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  Contact information for paper mail:
 *  Texas Instruments
 *  Post Office Box 655303
 *  Dallas, Texas 75265
 *  Contact information:
 *  http://www-k.ext.ti.com/sc/technical-support/product-information-centers.htm?
 *  DCMP=TIHomeTracking&HQS=Other+OT+home_d_contact
 *  ============================================================================
 *
 */

/**
 *******************************************************************************
 *  @file  Fb_blending.c
 *  @brief This file contains all Functions related to fbdev Application
 *
 *         This is the example IL Client support to create, configure & chaining
 *         of single channel omx-components using  non tunneling 
 *         mode
 *
 *  @rev 1.0
 *******************************************************************************
 */

/*******************************************************************************
*                             Compilation Control Switches
*******************************************************************************/
/* None */

/*******************************************************************************
*                             INCLUDE FILES
*******************************************************************************/

/*--------------------- system and platform files ----------------------------*/
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <linux/fb.h>
#include <linux/ti81xxfb.h>

#include <pthread.h>
/*-------------------------program files -------------------------------------*/
#include "ilclient.h"

/*
 * Macros Definations
 */
#define MAXLOOPCOUNT  1000
#define MAX_BUFFER    2

/*
 *  fbdev device nodes
 */
static char display_dev_name_fb0[30] = {"/dev/fb0"};
static char display_dev_name_fb1[30] = {"/dev/fb1"};
static char alpha;

/*
 * Color bar
 */
static int32_t rgb[8] =
{
  (0xFF << 16) | (0xFF << 8) | (0xFF),
  (0x00 << 16) | (0x00 << 8) | (0x00),
  (0xFF << 16) | (0x00 << 8) | (0x00),
  (0x00 << 16) | (0xFF << 8) | (0x00),
  (0x00 << 16) | (0x00 << 8) | (0xFF),
  (0xFF << 16) | (0xFF << 8) | (0x00),
  (0xFF << 16) | (0x00 << 8) | (0xFF),
  (0x00 << 16) | (0xFF << 8) | (0xFF),
};

/* ========================================================================== */
/**
* fill_color_bar() : This method is used to generate the color bar
*
* @param addr        : Address of the buffer.
* @param width       : Width in pixels
* @param height      : Height 
*
*  @return      
*         none
*
*/
/* ========================================================================== */
void fill_color_bar (unsigned char *addr, int width, int height)
{
  int32_t i, j, k;
  unsigned int *start = (unsigned int *) addr;

  if (width & 0xF)
  {
    width += 16 - (width & 0xF);
  }

  for (i = 0; i < 8; i++)
  {
    for (j = 0; j < (height / 8); j++)
    {
      for (k = 0; k < (width / 4); k++)
      {
        start[k] = rgb[i] | (alpha << 24);
      } /* for (k) */
      start += (width / 4);
    } /* for (j) */
  } /* for (i) */
} /* fill_color_bar */

/* ========================================================================== */
/**
* get_fixinfo() : This method is used to get the screen fix information
*
* @param display_fd        : Address of the buffer.
* @param fb_fix_screeninfo       : Width in pixels
*
*  @return
*         0
*/
/* ========================================================================== */
int32_t get_fixinfo (int32_t display_fd, struct fb_fix_screeninfo *fixinfo)
{
  int32_t ret;

  ret = ioctl (display_fd, FBIOGET_FSCREENINFO, fixinfo);
  if (ret < 0)
  {
    perror ("Error reading fixed information.\n");
    return ret;
  }
  printf ("\nFix Screen Info:\n");
  printf ("----------------\n");
  printf ("Line Length - %d\n", fixinfo->line_length);
  printf ("Physical Address = %lx\n", fixinfo->smem_start);
  printf ("Buffer Length = %d\n", fixinfo->smem_len);

  return 0;
} /* get_fixinfo */

/* ========================================================================== */
/**
* get_varinfo() : This method is used to get the variable information
*
* @param display_fd        : Address of the buffer.
* @param fb_fix_screeninfo       : Width in pixels
*
*  @return
*         0
*/
/* ========================================================================== */
int32_t get_varinfo (int32_t display_fd, struct fb_var_screeninfo *varinfo)
{
  int32_t ret;

  ret = ioctl (display_fd, FBIOGET_VSCREENINFO, varinfo);
  if (ret < 0)
  {
    perror ("Error reading variable information.\n");
    return ret;
  }
  printf ("\nVar Screen Info:\n");
  printf ("----------------\n");
  printf ("Xres - %d\n", varinfo->xres);
  printf ("Yres - %d\n", varinfo->yres);
  printf ("Xres Virtual - %d\n", varinfo->xres_virtual);
  printf ("Yres Virtual - %d\n", varinfo->yres_virtual);
  printf ("nonstd       - %d\n", varinfo->nonstd);
  printf ("Bits Per Pixel - %d\n", varinfo->bits_per_pixel);
  printf ("blue lenth %d msb %d offset %d\n",
          varinfo->blue.length, varinfo->blue.msb_right, varinfo->blue.offset);
  printf ("red lenth %d msb %d offset %d\n",
          varinfo->red.length, varinfo->red.msb_right, varinfo->red.offset);
  printf ("green lenth %d msb %d offset %d\n",
          varinfo->green.length,
          varinfo->green.msb_right, varinfo->green.offset);
  printf ("trans lenth %d msb %d offset %d\n",
          varinfo->transp.length,
          varinfo->transp.msb_right, varinfo->transp.offset);
  return 0;
} /* get_varinfo */

/* ========================================================================== */
/**
* get_regparams() : This method is used to get the register params
*
* @param display_fd        : display device
* @param ti81xxfb_region_params : Region params structure
*
*  @return
*         0
*/
/* ========================================================================== */

int32_t get_regparams (int display_fd, struct ti81xxfb_region_params *regp)
{
  int32_t ret;

  ret = ioctl (display_fd, TIFB_GET_PARAMS, regp);
  if (ret < 0)
  {
    perror ("Can not get region params\n");
    return ret;
  }
  printf ("\nReg Params Info:  \n");
  printf ("----------------\n");
  printf ("region %d postion %d x %d prioirty %d\n",
          regp->ridx, regp->pos_x, regp->pos_y, regp->priority);
  printf ("first %d last %d\n", regp->firstregion, regp->lastregion);
  printf ("sc en %d sten en %d\n", regp->scalaren, regp->stencilingen);
  printf ("tran en %d, type %d, key %d\n",
          regp->transen, regp->transtype, regp->transcolor);
  printf ("blend %d alpha %d\n", regp->blendtype, regp->blendalpha);
  printf ("bb en %d alpha %d\n", regp->bben, regp->bbalpha);
  return 0;
}

/* ========================================================================== */
/**
* IL_ClientFbDevAppTask() : This method is used to demo the fbdev capability
*
* @param threadsArg        : Not used.
*
*  @return
*         None
*/
/* ========================================================================== */

void IL_ClientFbDevAppTask (void *threadsArg)
{
  struct fb_fix_screeninfo fixinfo;
  struct fb_var_screeninfo varinfo, org_varinfo;
  struct ti81xxfb_region_params regp, oldregp;
  int32_t display_fd;
  //struct timeval before, after, result;
  int32_t ret = 0;
  int32_t buffersize;
  int32_t i;
  uint8_t *buffer_addr[MAX_BUFFER];
  int32_t dummy;
  IL_CLIENT_GFX_PRIVATE *pGfxData = NULL;

  pGfxData = (IL_CLIENT_GFX_PRIVATE *) threadsArg;
  alpha = 80;
  
  if (0 == pGfxData->gfxId)
  {
    /*As Gfx node is 0 is chosen FB0 is to be opened*/
    display_fd = open (display_dev_name_fb0, O_RDWR);
  }
  else
  {
    /*As Gfx node is 1 is chosen FB1 is to be opened*/  
    display_fd = open (display_dev_name_fb1, O_RDWR);
  }
  

  if (display_fd <= 0)
  {
    perror ("Could not open device\n");
    return;
  }
  /* Get fix screen information. Fix screen information gives * fix information 
     like panning step for horizontal and vertical * direction, line length,
     memory mapped start address and length etc. */

  if (get_fixinfo (display_fd, &fixinfo))
  {
    goto exit1;
  }
  /* Get variable screen information. Variable screen information * gives
     informtion like size of the image, bites per pixel, * virtual size of the
     image etc. */
  if (get_varinfo (display_fd, &varinfo))
  {
    goto exit1;
  }

  memcpy (&org_varinfo, &varinfo, sizeof (varinfo));

  if (get_regparams (display_fd, &regp))
  {
    goto exit1;
  }

  if (0 == pGfxData->gfxId)
  {
    /*Setting the Colorbar window size to smaller than the 1080P size*/
    varinfo.xres = 1280;
    varinfo.yres = 720;
  }
  else
  {
    /*Setting the Colorbar window size to smaller than the LCD screen size*/  
    varinfo.xres = 530;
    varinfo.yres = 320;
  }
  varinfo.xres_virtual = varinfo.xres;
  varinfo.yres_virtual = varinfo.yres * 2;

  /* set the ARGB8888 format */
  varinfo.red.length = varinfo.green.length
    = varinfo.blue.length = varinfo.transp.length = 8;
  varinfo.red.msb_right = varinfo.blue.msb_right =
    varinfo.green.msb_right = varinfo.transp.msb_right = 0;

  varinfo.red.offset = 16;
  varinfo.green.offset = 8;
  varinfo.blue.offset = 0;
  varinfo.transp.offset = 24;
  varinfo.bits_per_pixel = 32;

  ret = ioctl (display_fd, FBIOPUT_VSCREENINFO, &varinfo);
  if (ret < 0)
  {
    perror ("Error setting variable information.\n");
    goto exit1;
  }

  if (get_varinfo (display_fd, &varinfo))
  {
    goto exit2;
  }

  if (get_fixinfo (display_fd, &fixinfo))
  {
    goto exit2;
  }

  if (get_regparams (display_fd, &regp))
  {
    goto exit2;
  }

  oldregp = regp;

  /* Mmap the driver buffers in application space so that application can write 
     on to them. Driver allocates contiguous memory for three buffers. These
     buffers can be displayed one by one. */

  buffersize = fixinfo.line_length * varinfo.yres;
  buffer_addr[0] = (unsigned char *)
                     mmap (0, buffersize * MAX_BUFFER, (PROT_READ | PROT_WRITE),
                           MAP_SHARED, display_fd, 0);

  if ((int32_t) buffer_addr[0] == -1)
  {
    printf ("MMap Failed.\n");
    goto exit3;
  }
  /* Store each buffer addresses in the local variable. These buffer addresses
     can be used to fill the image. */

  for (i = 1; i < MAX_BUFFER; i++)
  {
    buffer_addr[i] = buffer_addr[i - 1] + buffersize;
  }

  /* fill buffer with color bar pattern */
  for (i = 0; i < MAX_BUFFER; i++)
  {
    fill_color_bar (buffer_addr[i], fixinfo.line_length, varinfo.yres);
  }

  i = 0;
  while (1)
  {
    ret = ioctl (display_fd, FBIOGET_VSCREENINFO, &varinfo);
    if (ret < 0)
    {
      perror ("ger varinfo failed.\n");
      break;
    }
    if (i % 128 == 0)
    {
      if (regp.blendtype == TI81XXFB_BLENDING_NO)
      {
        regp.blendtype = TI81XXFB_BLENDING_PIXEL;
      }
      else
      {
        regp.blendtype = TI81XXFB_BLENDING_NO;
      }
      ret = ioctl (display_fd, TIFB_SET_PARAMS, &regp);
      if (ret < 0)
      {
        perror ("can not set params.\n");
        break;
      }
    }
    ret = ioctl (display_fd, FBIO_WAITFORVSYNC, &dummy);
    if (ret < 0)
    {
      perror ("wait vsync failed.\n");
      break;
    }
    i++;
    if (pGfxData->terminateGfx)
    {
      break;
    }
  }
  munmap (buffer_addr[0], buffersize * MAX_BUFFER);

exit3:
  ret = ioctl (display_fd, TIFB_SET_PARAMS, &oldregp);
  if (ret < 0)
  {
    perror ("Error set reg params.\n");
  }

exit2:
  ret = ioctl (display_fd, FBIOPUT_VSCREENINFO, &org_varinfo);
  if (ret < 0)
  {
    perror ("Error reading variable information.\n");
  }
exit1:
  close (display_fd);
  pGfxData->terminateDone = 1;

  pthread_exit (threadsArg);

} /* IL_ClientFbDevAppTask */

/*Fb_blending.c - EOF*/
