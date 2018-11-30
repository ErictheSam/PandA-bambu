/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* lroundf adapted to be llroundf for Newlib, 2009 by Craig Howland.  */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include "math_privatef.h"

long long int llroundf(float x)
{
   int exponent_less_127;
   unsigned w;
   long long int result;
   int sign;

   GET_FLOAT_WORD(w, x);
   exponent_less_127 = ((w & 0x7f800000) >> 23) - 127;
   sign = (w & 0x80000000) != 0 ? -1 : 1;
   w &= 0x7fffff;
   w |= 0x800000;

   if(exponent_less_127 < (int)((8 * sizeof(long long int)) - 1))
   {
      if(exponent_less_127 < 0)
         return exponent_less_127 < -1 ? 0 : sign;
      else if(exponent_less_127 >= 23)
         result = (long long int)w << (exponent_less_127 - 23);
      else
      {
         w += 0x400000 >> exponent_less_127;
         result = w >> (23 - exponent_less_127);
      }
   }
   else
      return (long long int)x;

   return sign * result;
}

#ifdef _DOUBLE_IS_32BITS

long long int llround(double x)
{
   return llroundf((float)x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
