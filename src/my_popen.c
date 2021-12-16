#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>

#include "my_popen.h"


#ifndef WIN32

FILE *my_popen(char *cmd)
{
    static struct sigaction sa, restore_pipe_sa;
    FILE *result;

    // We must make sure PIPE is set to terminate process when we close
    // the handle to the unrar child
    sa.sa_flags = 0;
    sigemptyset (&sa.sa_mask);
    sa.sa_handler = SIG_DFL;
    sigaction (SIGPIPE, &sa, &restore_pipe_sa);

    result = popen(cmd, "r");

    // Restore signal
    sigaction(SIGPIPE, &restore_pipe_sa, NULL);

    return result;
}


int my_pclose(FILE *iofd)
{
    return pclose(iofd);
}



#else  // WIN32


// These come from win32_popen.c
FILE  *safe_popen  (const char *cm, const char *md);
int    safe_pclose (FILE *pp);


FILE *my_popen(char *cmd)
{
	return safe_popen(cmd, "rb");
}


int my_pclose(FILE *iofd)
{
	return safe_pclose(iofd);
}

#endif



