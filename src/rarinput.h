#ifndef RARINPUT_H_INCLUDED
#define RARINPUT_H_INCLUDED

#include "dvdread/dvd_udf.h"
/* When is it faster to respawn unrar, rather than seek by eating bytes? */
#define MAXIMUM_SEEK_SIZE 1048576

struct rarinput_dev_struct {
    /* unrar support */
    char *unrar_archive;
    char *unrar_file;
    FILE *unrar_stream;
    off_t seek_pos;
    off_t current_pos;
};

//typedef struct rarinput_dev_struct rarinput_dev_t;
typedef struct rarinput_dev_struct *dvd_rinput_t;

extern char *rarinput_name_in_rar;



dvd_rinput_t rarinput_open (const char *target);
char        *rarinput_error(dvd_rinput_t dev);
int          rarinput_seek (dvd_rinput_t dev, int blocks);
int          rarinput_title(dvd_rinput_t dev, int block);
int          rarinput_read (dvd_rinput_t dev, void *buffer, int blocks, int flags);
int          rarinput_close(dvd_rinput_t dev);



#endif
