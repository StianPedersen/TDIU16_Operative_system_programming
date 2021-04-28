
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>


int main()
{
int child = exec("sumargv 1 2 3"); // Starta en barnprocess som summerar argumenten
int result = wait(child); // Vänta på att den blev klar och hämta resultatet
printf("Sum: %d\n", result); // Skriv ut det.
return 0;
}
