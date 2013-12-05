#include <stdio.h>
 
long factorial(int);
 
int gcd ( int a, int b )
{
  int c;
  while ( a != 0 ) {
     c = a; a = b%a;  b = c;
  }
  return b;
}

/* Recursive Standard C Function: Greatest Common Divisor */
int gcdr ( int a, int b )
{
  if ( a==0 ) return b;
  return gcdr ( b%a, a );
}

int main()
{
  int number;
  long fact = 1;
  int d;
  for (d = 0; d < 1000000000000000000000000000000; ++d)
  {
     number = fact+1;
     printf("%d! = %ld\n", number, factorial(number));
     number = gcd(gcdr(number,gcdr(number,fact*5)*5),fact*3);
     printf("gcd %d", number);
     fact++;

     int n, i = 3, count, c;
 
     n = fact;
   
     if ( n >= 1 )
     {
        printf("First %d prime numbers are :\n",n);
        printf("2\n");
     }
   
     for ( count = 2 ; count <= n ;  )
     {
        for ( c = 2 ; c <= i - 1 ; c++ )
        {
           if ( i%c == 0 )
              break;
        }
        if ( c == i )
        {
           printf("%d\n",i);
           count++;
        }
        i++;
     }
  }

 
 
  return 0;
}
 
long factorial(int n)
{
  int c = 0;
  long result = 1;
 
  for (c = 1; c <= n; c++)
    result = result * c;
   printf("%ld \n",result);
  return result;
}