#include <stdio.h>

int main( int argc, char *argv[] ) 
{
  printf("Hello World!\n");
  // lets do some counting
  count( 1000 );
  return 0;
}

// count to max and then return max
int count( int max ) 
{
  for (int i = 0; i <= max; i++) 
  {
    printf("%d,", i); 
  }
  printf("\n");
  return count;
}

 
