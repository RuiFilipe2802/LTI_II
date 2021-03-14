#include <stdio.h>
#include "bit-ops.h"

void print_bits(unsigned char x)
{
   int i;
   for (i = 8 * sizeof(x) - 1; i >= 0; i--)
   {
      (x & (1 << i)) ? putchar('1') : putchar('0');
   }
   printf("\n");
}

int main(int argc, char *argv[])
{
   char A[2];
   int i;

   for (i = 0; i < 1; i++)
   {
      A[i] = 0; // Clear the bit array
   }

   printf("Set bit poistions 0, 2 and 31\n");
   SetBit(A, 0); // Set 3 bits
   SetBit(A, 2);
   SetBit(A, 15);

   for (size_t i = 0; i < 2; i++)
   {
      print_bits(A[i]);
   }

   // Check if SetBit() works:

   for (i = 0; i < 16; i++)
   {
      if (TestBit(A, i))
         printf("Bit %d was set !\n", i);
   }

   printf("\nClear bit poistions 0 \n");
   ClearBit(A, 0);

   // Check if ClearBit() works:
   for (size_t i = 0; i < 2; i++)
   {
      print_bits(A[i]);
   }
   for (i = 0; i < 16; i++)
   {
      if (TestBit(A, i))
         printf("Bit %d was set !\n", i);
   }
}