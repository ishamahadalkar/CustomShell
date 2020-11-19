
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>

struct termios tty_attr;

/* 
 * Sets terminal into raw mode. 
 * This causes having the characters available
 * immediately instead of waiting for a newline. 
 * Also there is no automatic echo.
 */

void tty_raw_mode(void) {
	struct termios newtty_attr;
     
	tcgetattr(0, &tty_attr);
	tcgetattr(0,&newtty_attr);

	/* Set raw mode. */
	newtty_attr.c_lflag &= (~(ICANON|ECHO));
	newtty_attr.c_cc[VTIME] = 0;
	newtty_attr.c_cc[VMIN] = 1;
     
	tcsetattr(0,TCSANOW,&newtty_attr);
}

/* 
 * Resets the terminal
 */

void reset(void)
{
	tcsetattr(0,TCSANOW,&tty_attr);
}