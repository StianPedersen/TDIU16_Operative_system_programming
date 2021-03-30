#include <stdio.h>
#include <string.h>

int main( int argc, char *argv[] )
{
  int summation{0};
  int maxwidth = 21;
  for (int i = 0 ; i < argc ; ++i)
  {
    int width = maxwidth - (int)strlen(argv[i]);
    printf("%s %*i \n", argv[i],width, (int)strlen(argv[i]) );
    summation = (int)strlen(argv[i]) + summation;
  }
  int width = maxwidth - (int)strlen("Total length");
  printf("%s %*i \n", "Total length", width, summation );
  width = maxwidth - (int)strlen("Avarage length");
  printf("%s %*.2f \n", "Avarage length",width, ((double)summation/(double)argc) );
  return summation;
}
