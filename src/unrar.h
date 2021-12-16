#ifndef UNRAR_H_INCLUDED
#define UNRAR_H_INCLUDED


#define NOT_RAR 0
#define  IS_RAR 1
#define  IS_ISO 2

#ifndef WIN32
#define UNRAR_CMD "unrar"
#else
#define UNRAR_CMD "unrar.exe"
#endif



int      unrar_parse   ( char *, char *, uint64_t *, char ** );
int      unrar_list    ( char *, char * );
uint64_t unrar_getsize ( char *, char *, char ** );
FILE    *unrar_open    ( char *, char *, int );
int      unrar_read    ( unsigned char *, int , FILE *, int );
int      unrar_seek    ( FILE **, MediaInfo_int64u , char *,
                         char *, int );
int      unrar_close   ( FILE *, int );
uint64_t unrar_tell    ( void );
#endif
