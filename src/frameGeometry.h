#ifndef FRAMEGEOMETRY_H
#define FRAMEGEOMETRY_H

#include "defines.h"
#include "types.h"

class FrameGeometry
{
public:
	unsigned int hRes;		/* Horizontal active pixels. */
	unsigned int vRes;		/* Vertical active pixels. */
	unsigned int hOffset;	/* Horizontal offset from first active column. */
	unsigned int vOffset;	/* Vertical offset from first active row. */
	unsigned int vDarkRows;	/* Number of vertical dark rows read out before active pixels. */
	unsigned int bitDepth;	/* Bits per pixel. */

	/* Some Helper routines */
	unsigned long pixels() const { return (hRes * (vRes + vDarkRows)); }
	unsigned long size() const { return (bitDepth * pixels()) / 8; }
};

#endif // FRAMEGEOMETRY_H
