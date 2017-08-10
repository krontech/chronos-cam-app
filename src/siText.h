#include "types.h"


#define SI_DELIM_SPACE			1
#define SI_DELIM_COMMA			2
#define SI_SPACE_BEFORE_PREFIX	4

void getSIText(char * buf, double val, UInt32 sigfigs, UInt32 options, Int32 resolution);
double siText2Double(const char * textIn);
