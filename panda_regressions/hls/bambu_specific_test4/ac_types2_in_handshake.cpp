#include "ap_int.h"

#pragma HLS_interface a handshake
#pragma HLS_interface b handshake
#pragma HLS_interface c handshake
void sum3numbers(ap_int<67> *a, ap_int<67> *b, ap_int<67> *c, ap_int<68> *d)  
{
  *d = *a + *b + *c;
}
