/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/elk/RCS/init.c,v 1.0 1997/05/06 20:44:35 schenney Exp $
 *
 * $Log: init.c,v $
 * Revision 1.0  1997/05/06 20:44:35  schenney
 * Initial revision
 *
 */
#include "elk_private.h"
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

extern int Elk_Init_Eval();
extern int Elk_Init_Callbacks();

int
Elk_Sced_Init(int ac, char **av)
{
    int fakeac = 1;
    char *fakeav[1];
    Boolean had_init = FALSE;

    elk_active = ELK_MAIN_WINDOW;
    elk_window = &main_window;

    fakeav[0] = av[0];

    if ( ! access("init.scm", R_OK) )
    {
        Elk_Init(fakeac, fakeav, 0, "init.scm");
        had_init = TRUE;
    }
    else
        Elk_Init(fakeac, fakeav, 0, NULL);
    Elk_Init_Eval();
    /*
     * Now we define various functional interfaces for calling
     * from scheme to C
     */
    Elk_Init_Callbacks();
    if ( had_init )
    {
	Elk_Create_Window();
        Elk_Echo_Output(Elk_Eval("(startup)\n"));
    }
    return 0;
}
