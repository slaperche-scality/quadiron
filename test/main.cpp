
#include "ntl.h"

extern void gf_utest();
extern void rs_utest();
extern void mat_utest();
extern void fft_utest();

int main(int argc, char **argv)
{
  gf_utest();
  mat_utest();
  rs_utest();
  fft_utest();
}
