/*									tab:8
 *
 * input.c - source file for input control to maze game
 *
 * "Copyright (c) 2004-2011 by Steven S. Lumetta."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE AUTHOR OR THE UNIVERSITY OF ILLINOIS BE LIABLE TO 
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL 
 * DAMAGES ARISING OUT  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, 
 * EVEN IF THE AUTHOR AND/OR THE UNIVERSITY OF ILLINOIS HAS BEEN ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE AUTHOR AND THE UNIVERSITY OF ILLINOIS SPECIFICALLY DISCLAIM ANY 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE 
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND NEITHER THE AUTHOR NOR
 * THE UNIVERSITY OF ILLINOIS HAS ANY OBLIGATION TO PROVIDE MAINTENANCE, 
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Author:	    Steve Lumetta
 * Version:	    7
 * Creation Date:   Thu Sep  9 22:25:48 2004
 * Filename:	    input.c
 * History:
 *	SL	1	Thu Sep  9 22:25:48 2004
 *		First written.
 *	SL	2	Sat Sep 12 14:34:19 2009
 *		Integrated original release back into main code base.
 *	SL	3	Sun Sep 13 03:51:23 2009
 *		Replaced parallel port with Tux controller code for demo.
 *	SL	4	Sun Sep 13 12:49:02 2009
 *		Changed init_input order slightly to avoid leaving keyboard
 *              in odd state on failure.
 *	SL	5	Sun Sep 13 16:30:32 2009
 *		Added a reasonably robust direct Tux control for demo mode.
 *	SL	6	Wed Sep 14 02:06:41 2011
 *		Updated input control and test driver for adventure game.
 *	SL	7	Wed Sep 14 17:07:38 2011
 *		Added keyboard input support when using Tux kernel mode.
 */

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
#include <termio.h>
#include <termios.h>
#include <unistd.h>

#include "assert.h"
#include "input.h"
#include "module/tuxctl-ioctl.h"

/* set to 1 and compile this file by itself to test functionality */
#define TEST_INPUT_DRIVER 0

/* set to 1 to use tux controller; otherwise, uses keyboard input */
#define USE_TUX_CONTROLLER 1


/* stores original terminal settings */
static struct termios tio_orig;

/* stores our previous command from the current call */
cmd_t prevcmd;

/* 
 * init_input
 *   DESCRIPTION: Initializes the input controller.  As both keyboard and
 *                Tux controller control modes use the keyboard for the quit
 *                command, this function puts stdin into character mode
 *                rather than the usual terminal mode.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure 
 *   SIDE EFFECTS: changes terminal settings on stdin; prints an error
 *                 message on failure
 */
int
init_input ()
{
    struct termios tio_new;

    /*
     * Set non-blocking mode so that stdin can be read without blocking
     * when no new keystrokes are available.
     */
    if (fcntl (fileno (stdin), F_SETFL, O_NONBLOCK) != 0) {
        perror ("fcntl to make stdin non-blocking");
	return -1;
    }

    /*
     * Save current terminal attributes for stdin.
     */
    if (tcgetattr (fileno (stdin), &tio_orig) != 0) {
	perror ("tcgetattr to read stdin terminal settings");
	return -1;
    }

    /*
     * Turn off canonical (line-buffered) mode and echoing of keystrokes
     * to the monitor.  Set minimal character and timing parameters so as
     * to prevent delays in delivery of keystrokes to the program.
     */
    tio_new = tio_orig;
    tio_new.c_lflag &= ~(ICANON | ECHO);
    tio_new.c_cc[VMIN] = 1;
    tio_new.c_cc[VTIME] = 0;
    if (tcsetattr (fileno (stdin), TCSANOW, &tio_new) != 0) {
	perror ("tcsetattr to set stdin terminal settings");
	return -1;
    }

    // Commands to initialize connection to the tux controller
    fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY);
	int ldsic_num = N_MOUSE;
	ioctl(fd, TIOCSETD, &ldsic_num);

	// Initialize our tux controller
	ioctl(fd, TUX_INIT);
    /* Return success. */
    return 0;
}

static char typing[MAX_TYPED_LEN + 1] = {'\0'};

const char*
get_typed_command ()
{
    return typing;
}

void
reset_typed_command ()
{
    typing[0] = '\0';
}

static int32_t
valid_typing (char c)
{
    /* Valid typing include letters, numbers, space, and backspace/delete. */
    return (isalpha (c) || isdigit (c) || ' ' == c || 8 == c || 127 == c);
}

static void
typed_a_char (char c)
{
    int32_t len = strlen (typing);

    if (8 == c || 127 == c) {
        if (0 < len) {
	    typing[len - 1] = '\0';
	}
    } else if (MAX_TYPED_LEN > len) {
	typing[len] = c;
	typing[len + 1] = '\0';
    }
}

/* 
 * get_tux_command
 *   DESCRIPTION: Reads a command from the tux controller in the form
 *				  of a hexadecimal value. ioctl helps us decipher this
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: command issued by the tux controller
 *   SIDE EFFECTS: drains any tux input
 */
cmd_t
get_tux_command()
{
	unsigned char button_press = 0;

	// Grabs the button pressed from the tux controller
	ioctl(fd, TUX_BUTTONS, &button_press);
	cmd_t pushed;	

	// Chooses which cmd_t command the tux controller pressed
	switch(button_press)
		{
			case 0xFE:
				pushed = CMD_QUIT;
				break;
			case 0xFD:
				pushed = CMD_MOVE_LEFT;
				break;
			case 0xFB:
				pushed = CMD_ENTER;
				break;
			case 0xF7:
				pushed = CMD_MOVE_RIGHT;
				break;
			case 0xEF:
				pushed = CMD_UP;
				break;
			case 0xBF:
				pushed = CMD_LEFT;
				break;
			case 0xDF:
				pushed = CMD_DOWN;
				break;
			case 0x7F:
				pushed = CMD_RIGHT;
				break;
			default:
				pushed = CMD_NONE;
		}

	// This is to prevent flickering and spamming of a command that changes the screen.
	// Because the CMD_NONE follows any button press, we use this to prevent spamming
	if (pushed == CMD_NONE)
		prevcmd = CMD_NONE;
	else if (pushed == prevcmd && (pushed == CMD_MOVE_RIGHT || pushed == CMD_MOVE_LEFT || pushed == CMD_ENTER))
		pushed = CMD_NONE;
	else
		prevcmd = pushed;

	// lets return our command!
	return pushed;
}

/* 
 * get_command
 *   DESCRIPTION: Reads a command from the keyboard controller.  As some
 *                controllers provide only absolute input (e.g., go
 *                right), the current direction is needed as an input
 *                to this routine.
 *   INPUTS: cur_dir -- current direction of motion
 *   OUTPUTS: none
 *   RETURN VALUE: command issued by the keyboard controller
 *   SIDE EFFECTS: drains any keyboard input
 */
cmd_t 
get_command ()
{
//#if (USE_TUX_CONTROLLER == 0) /* use keyboard control with arrow keys */
    static int state = 0;             /* small FSM for arrow keys */
//#endif
    static cmd_t command = CMD_NONE;
    cmd_t pushed = CMD_NONE;
    int ch;

    /* Read all characters from stdin. */
    while ((ch = getc (stdin)) != EOF) {

	/* Backquote is used to quit the game. */
	if (ch == '`')
	    return CMD_QUIT;
	
//#if (USE_TUX_CONTROLLER == 0) /* use keyboard control with arrow keys */
	/*
	 * Arrow keys deliver the byte sequence 27, 91, and 'A' to 'D';
	 * we use a small finite state machine to identify them.
	 *
	 * Insert, home, and page up keys deliver 27, 91, '2'/'1'/'5' and
	 * then a tilde.  We recognize the digits and don't check for the
	 * tilde.
	 */
	switch (state) {
	    case 0:
	        if (27 == ch) {
		    state = 1;
		} else if (valid_typing (ch)) {
		    typed_a_char (ch);
		} else if (10 == ch || 13 == ch) {
		    pushed = CMD_TYPED;
		}
		break;
	    case 1:
		if (91 == ch) {
		    state = 2;
		} else {
		    state = 0;
		    if (valid_typing (ch)) {
			/*
			 * Note that we may be discarding an ESC (27), but
			 * we don't use that as typed input anyway.
			 */
			typed_a_char (ch);
		    } else if (10 == ch || 13 == ch) {
			pushed = CMD_TYPED;
		    }
		}
		break;
	    case 2:
	        if (ch >= 'A' && ch <= 'D') {
		    switch (ch) {
			case 'A': pushed = CMD_UP; break;
			case 'B': pushed = CMD_DOWN; break;
			case 'C': pushed = CMD_RIGHT; break;
			case 'D': pushed = CMD_LEFT; break;
		    }
		    state = 0;
		} else if (ch == '1' || ch == '2' || ch == '5') {
		    switch (ch) {
			case '2': pushed = CMD_MOVE_LEFT; break;
			case '1': pushed = CMD_ENTER; break;
			case '5': pushed = CMD_MOVE_RIGHT; break;
		    }
		    state = 3; /* Consume a '~'. */
		} else {
		    state = 0;
		    if (valid_typing (ch)) {
			/*
			 * Note that we may be discarding an ESC (27) and 
			 * a bracket (91), but we don't use either as 
			 * typed input anyway.
			 */
			typed_a_char (ch);
		    } else if (10 == ch || 13 == ch) {
			pushed = CMD_TYPED;
		    }
		}
		break;
	    case 3:
		state = 0;
	        if ('~' == ch) {
		    /* Consume it silently. */
		} else if (valid_typing (ch)) {
		    typed_a_char (ch);
		} else if (10 == ch || 13 == ch) {
		    pushed = CMD_TYPED;
		}
		break;
	}
//#else /* USE_TUX_CONTROLLER */
	/* Tux controller mode; still need to support typed commands. */



/*
	This part is commented out, but used for our solo input function running for the
	tux controller. IT can be uncommented out to use the main function below.
*/

//#endif  USE_TUX_CONTROLLER 
    }
/*
	unsigned char button_press = 0;
	ioctl(fd, TUX_BUTTONS, &button_press);
	//cmd_t pushed;

	//printf("%x\n", (int)button_press);	

	switch(button_press)
		{
			case 0xFE:
				pushed = CMD_QUIT;
			//	printf("QUIT\n");
				break;
			case 0xFD:
				pushed = CMD_MOVE_LEFT;
			//	printf("LEFT\n");
				break;
			case 0xFB:
				pushed = CMD_ENTER;
			//	printf("ENTER\n");
				break;
			case 0xF7:
				pushed = CMD_MOVE_RIGHT;
			//	printf("RIGHT\n");
				break;
			case 0xEF:
				pushed = CMD_UP;
			//	printf("UP\n");
				break;
			case 0xBF:
				pushed = CMD_LEFT;
			//	printf("LEFT\n");
				break;
			case 0xDF:
				pushed = CMD_DOWN;
			//	printf("DOWN\n");
				break;
			case 0x7F:
				pushed = CMD_RIGHT;
			//	printf("RIGHT\n");
				break;
			default:
				pushed = CMD_NONE;
		}
*/

    /*
     * Once a direction is pushed, that command remains active
     * until a turn is taken.
     */
    if (pushed == CMD_NONE) {
        command = CMD_NONE;
    }
    return pushed;
}

/* 
 * shutdown_input
 *   DESCRIPTION: Cleans up state associated with input control.  Restores
 *                original terminal settings.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none 
 *   SIDE EFFECTS: restores original terminal settings
 */
void
shutdown_input ()
{
    (void)tcsetattr (fileno (stdin), TCSANOW, &tio_orig);
}

/* 
 * display_time_on_tux
 *   DESCRIPTION: Show number of elapsed seconds as minutes:seconds
 *                on the Tux controller's 7-segment displays.
 *   INPUTS: num_seconds -- total seconds elapsed so far
 *   OUTPUTS: none
 *   RETURN VALUE: none 
 *   SIDE EFFECTS: changes state of controller's display
 */
void
display_time_on_tux (int num_seconds)
{

	int ten_minutes, one_minutes, ten_seconds, one_seconds;
	unsigned long arg;

	one_seconds = num_seconds;
	arg = ten_minutes = one_minutes = ten_seconds = 0;

	// if time is more than 60 seconds we put it into minutes
	while(one_seconds >= 60)
	{
		one_seconds -= 60;
		one_minutes++;
	}

	// if time is more than 10 minutes we put into 10 minutes (4th led)
	while(one_minutes >= 10)
	{
		one_minutes -= 10;
		ten_minutes++;
	}

	// second led number
	while(one_seconds >= 10 && one_seconds < 60)
	{
		one_seconds -= 10;
		ten_seconds++;
	}

	// debug if statement
	if(one_seconds >= 10 || ten_seconds >= 10 || one_minutes >= 10 || ten_minutes >= 10)
	{
		// printf("some while loop error\n");
	}

	// setting 16 LSBs for arg
	arg = ((ten_minutes << 12) | (one_minutes << 8) | (ten_seconds << 4) | (one_seconds));

	if(ten_minutes > 0)
	{
		// turn on 4 leds and dec
		arg |= 0x040F0000;
	}

	else
	{
		// turn on 3 leds and dec
		arg |= 0x04070000;
	}

	// Call to set the LEDs
	ioctl(fd, TUX_SET_LED, arg);
	return;
}


#if (TEST_INPUT_DRIVER == 1)
int
main ()
{
    cmd_t last_cmd = CMD_NONE;
    cmd_t cmd;
    int i = 0;
    static const char* const cmd_name[NUM_COMMANDS] = {
        "none", "right", "left", "up", "down", 
	"move left", "enter", "move right", "typed command", "quit"
    };

    /* Grant ourselves permission to use ports 0-1023 */
    if (ioperm (0, 1024, 1) == -1) {
	perror ("ioperm");
	return 3;
    }

    // Used for TUX testing
/*	display_time_on_tux(5, fd);
	getchar();
	display_time_on_tux(50,fd);
	getchar();
	display_time_on_tux(500, fd);
	getchar();
	display_time_on_tux(5000, fd);
	getchar(); */

	// initialize our tux input    
	init_input ();

	// tests our commands and prints time on tux
    while (1) {
        while ((cmd = get_command()) == last_cmd);
	last_cmd = cmd;
	printf ("command issued: %s\n", cmd_name[cmd]);
	if (cmd == CMD_QUIT)
	    break;
	usleep(1000000);
	display_time_on_tux (i);
	i++;
    }
    shutdown_input ();
    return 0;
}
#endif

