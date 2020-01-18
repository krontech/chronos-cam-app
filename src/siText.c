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
#include "siText.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

char prefixes[17] = {'y', 'z', 'a', 'f', 'p', 'n', 'u', 'm', '\0', 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y'}; //10**0 is position 8



void getSIText(char * buf, double val, UInt32 sigfigs, UInt32 options, Int32 resolution)
{
	Int32 ex = 0;
	Int32 decPlaces;
	char negative = 0;

	if(val < 0)
	{
		val = -val;
		negative = 1;
	}

	if(val != 0)
		{
		if(val >= 10000.0)
		{
			do {
				val /= 1000.0;
				ex += 3;
			} while (val >= 1000.0 && ex <= 24);
		}
		else if(val <= 1.0)
		{
			do {
				val *= 1000.0;
				ex -= 3;
			} while (val < 1.0 && ex >= -24);
		}
	}

	//Get the number of places required after the decimal
	if(val >= 100.0)
		decPlaces = sigfigs - 3;
	else if(val >= 10.0)
		decPlaces = sigfigs - 2;
	else
		decPlaces = sigfigs - 1;

	//Maintain only a certian resolution
	if(decPlaces > (resolution + ex))
		decPlaces = resolution + ex;

	if(decPlaces < 0)
		decPlaces = 0;

	if(negative)
		val = -val;

	sprintf(buf, "%.*f%s%c", decPlaces, val, options & SI_SPACE_BEFORE_PREFIX ? " " : "",  prefixes[ex/3+8]);

	//Instert thousands separators if needed
	if(	options & SI_DELIM_SPACE ||
		options & SI_DELIM_COMMA)
	{
		char * pt;
		char * end;

		// Find the decimal point.
		for (pt = buf; *pt && *pt != '.'; pt++) {}
		// Skip the decimal point and the first three digits.
		pt += 4;
		decPlaces -= 3;

		// Find end (including terminator)
		end = buf + strlen(buf) + 1;
		// Step forwards from the decimal, inserting spaces
		while (decPlaces > 0) {
			// Insert a separator
			memmove(pt + 1, pt, end - pt + 1);
			*pt = (options & SI_DELIM_SPACE) ? ' ' : ','; // thousand separator
			pt += 4; // 3 digits + separator
			decPlaces -= 3;
		}
	}
}


double siText2Double(const char * textIn)
{
	const char * in = textIn;
	char txt[100];
	int ex = 0;
	int j;

	//Copy the input to a numeric text buffer, discarding any spaces or commas, and store the prefix
	int i = 0;
	while(*in)
	{
		if(isdigit(*in) || '.' == *in || '+' == *in || '-' == *in)
			txt[i++] = *in;
		else if(isalpha(*in))
		{
			//Scan the prefix list and fill the prefix if found
			for(j = 0; j < 17; j++)
				if(*in == prefixes[j])
				{
					ex = (j - 8) * 3;
					break;
				}
		}
		in++;
	}
	txt[i] = 0;	//add null terminator

	return atof(txt) * pow(10, ex);
}
