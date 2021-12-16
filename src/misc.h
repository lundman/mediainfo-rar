#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED



#ifdef  DEBUG
#define VERBOSE
#endif
#define IO_SIZE      8192
#define MAXIMUM_SEEK 1048576

#define SAFE_FREE(X) { if ((X)) { free((X)); (X) = NULL; } }


char    *misc_digtoken  ( char **string,char *match );


#endif
