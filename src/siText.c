#include "siText.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

char * prefixes[17] = {'y', 'z', 'a', 'f', 'p', 'n', 'u', 'm', '\0', 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y'}; //10**0 is position 8



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
		if(val >= 1000.0)
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

	sprintf(buf, "%.*f%c%c", decPlaces, val, options & SI_SPACE_BEFORE_PREFIX ? ' ' : '\0',  prefixes[ex/3+8]);

	//Instert thousands separators if needed
	if(	options & SI_DELIM_SPACE ||
		options & SI_DELIM_COMMA)
	{
		char * pt;
		char * end;

		// Point pt at "."
		for (pt = buf; *pt && *pt != '.'; pt++) {}
		//pt++;

		// Find end (including terminator)
		end = buf + strlen(buf);
		// Step forwards from the decimal, inserting spaces
		do {
			pt += 4; // shift 3 digits
			if (/*pt <= end*/decPlaces > 3) {
				memmove(pt + 1, pt, end - pt + 1);
				*pt = (options & SI_DELIM_SPACE) ? ' ' : ','; // thousand separator
				decPlaces -= 3;
				//n += 4; // 3 digits + separator
			} else
				break;
		} while (1);
	}
}


double siText2Double(const char * textIn)
{
	char * in = textIn;
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
