/**
 * strcmp primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
 */
/* glibc library */

int strcmp(const char* p1, const char* p2)
{
   const unsigned char* s1 = (const unsigned char*)p1;
   const unsigned char* s2 = (const unsigned char*)p2;
   unsigned char c1, c2;

   do
   {
      c1 = (unsigned char)*s1++;
      c2 = (unsigned char)*s2++;
      if(c1 == '\0')
         return c1 - c2;
   } while(c1 == c2);

   return c1 - c2;
}
