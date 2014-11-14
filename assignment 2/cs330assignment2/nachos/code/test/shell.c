#include "syscall.h"

int
main()
{
    SpaceId newProc;
    OpenFileId input = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char prompt[2], ch, buffer[60];
    int i;

    prompt[0] = '-';
    prompt[1] = '-';

    while( 1 )
    {
	sys_Write(prompt, 2, output);

	i = 0;
	
	do {
	
	    sys_Read(&buffer[i], 1, input); 

	} while( buffer[i++] != '\n' );

	buffer[--i] = '\0';

	/*if( i > 0 ) {
		newProc = sys_Exec(buffer);
		sys_Join(newProc);
	}*/
    }
}

