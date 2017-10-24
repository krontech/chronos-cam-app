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
#ifndef TYPES_H
#define TYPES_H


#define	UInt8	unsigned char
#define	UInt16	unsigned short
#define	UInt32	unsigned int
#define	UInt64	unsigned long long

#define	Int8	char
#define	Int16	short
#define	Int32	int
#define	Int64	long long


#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
	   __typeof__ (b) _b = (b); \
	 _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
	   __typeof__ (b) _b = (b); \
	 _a < _b ? _a : _b; })

#define within(x, mn, mx) \
   ({ __typeof__ (mn) _mn = (mn); \
	   __typeof__ (mx) _mx = (mx); \
		__typeof__ (x) _x = (x); \
	 _x < _mn ? _mn : _x > _mx ? _mx : _x; })

//Returns the lowest multiple of mult equal to or greater than x
#define ROUND_UP_MULT(x, mult)	(((x) + ((mult) - 1)) & ~((mult) - 1))

#endif // TYPES_H
