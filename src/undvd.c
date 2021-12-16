#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <errno.h>

#if MEDIAINFO_STATIC
#include <MediaInfoDLL/MediaInfoDLL_Static.h>
#else
#include <MediaInfoDLL/MediaInfoDLL.h>
#endif

#include "dvdread/dvd_reader.h"

#ifdef WIN32
#include "win32.h"
#endif

#include "my_popen.h"
#include "unrar.h"
#include "undvd.h"
#include "lfnmatch.h"
#include "misc.h"

extern int opt_verbose;

char *strjoin(char *a, char *b)
{
  char *result;
  int len_a, extra_slash;

  // Check if a ends with / or, b starts with /, if so dont
  // add a '/'.
  len_a = strlen(a);

  if ((a[ len_a - 1 ] != '/') && b[0] != '/')
    extra_slash = 1;
  else
    extra_slash = 0;



  result = (char *) malloc(len_a + strlen(b) + 1 + extra_slash);

  if (!result) {
    perror("malloc");
    exit(2);
  }

  if (extra_slash)
    sprintf(result, "%s/%s", a, b);
  else
    sprintf(result, "%s%s", a, b);

  return result;
}




void dvd_list_recurse(dvd_reader_t *dvdread, char *directory)
{
    dvd_dir_t *dvd_dir;
    dvd_dirent_t *dvd_dirent;
    char *path = NULL;

    dvd_dir = DVDOpenDir(dvdread, directory);
    if (!dvd_dir) return;

    while((dvd_dirent = DVDReadDir(dvdread, dvd_dir))) {

        if (!strcmp((char *)dvd_dirent->d_name, ".")) continue;
        if (!strcmp((char *)dvd_dirent->d_name, "..")) continue;

        // Is it a regular file?
        if (dvd_dirent->d_type == DVD_DT_REG) {

            if (!strcmp("/", directory))
                printf("-rw-r--r-- %12"PRIu64" %s\n",
                       dvd_dirent->d_filesize,
                       dvd_dirent->d_name);
            else
                printf("-rw-r--r-- %12"PRIu64" %s/%s\n",
                       dvd_dirent->d_filesize,
                       &directory[1],
                       dvd_dirent->d_name);

        } else if (dvd_dirent->d_type == DVD_DT_DIR) {

            if (!strcmp("/", directory))
                printf("drw-r--r-- %12"PRIu64" %s\n",
                       dvd_dirent->d_filesize,
                       dvd_dirent->d_name);
            else
                printf("drw-r--r-- %12"PRIu64" %s/%s\n",
                       dvd_dirent->d_filesize,
                       &directory[1],
                       dvd_dirent->d_name);

            path = strjoin(directory, (char *)dvd_dirent->d_name);
            dvd_list_recurse(dvdread, path);
            free(path); path = NULL;

        } // DIR

    } // while readdir

    DVDCloseDir(dvdread, dvd_dir);

} // dvd_dir

void dvd_get_recurse(dvd_reader_t *dvdread, char *directory, char *match,
                     char **biggest_name, uint64_t *biggest_size)
{
    dvd_dir_t *dvd_dir;
    dvd_dirent_t *dvd_dirent;
    char *path = NULL;

    //printf("get_recurse '%s'\n", directory);

    dvd_dir = DVDOpenDir(dvdread, directory);
    if (!dvd_dir) return;

    while((dvd_dirent = DVDReadDir(dvdread, dvd_dir))) {

        if (!strcmp((char *)dvd_dirent->d_name, ".")) continue;
        if (!strcmp((char *)dvd_dirent->d_name, "..")) continue;

        // Is it a regular file?
        if (dvd_dirent->d_type == DVD_DT_REG) {

            if (!lfnmatch(match,
                          (char *)dvd_dirent->d_name, LFNM_CASEFOLD)) {

                if (dvd_dirent->d_filesize > *biggest_size) {
                    *biggest_size = dvd_dirent->d_filesize;
                    if (biggest_name) {
                        SAFE_FREE(*biggest_name);
                        if (!strcmp("/", directory))
                            *biggest_name = strdup((char *)dvd_dirent->d_name);
                        else
                            *biggest_name = strjoin(&directory[1],
                                                    (char *)dvd_dirent->d_name);
                    }
                } // biggest in size
            } // match

        } else if (dvd_dirent->d_type == DVD_DT_DIR) {

            path = strjoin(directory, (char *)dvd_dirent->d_name);
            dvd_get_recurse(dvdread, path, match, biggest_name, biggest_size);
            free(path); path = NULL;

        } // DIR

    } // while readdir

    DVDCloseDir(dvdread, dvd_dir);

} // dvd_dir


int undvd_list(char *filename, char *argname)
{
	dvd_reader_t *dvdread = NULL;

    main_setup_dvdread();
    dvdread = DVDOpen(filename);
    if (!dvdread) return -1;

    dvd_list_recurse(dvdread, !argname ? "/" : argname);

    DVDClose(dvdread);

    return 0;
}


uint64_t undvd_getsize(char *filename, char *argname, char **biggestname)
{
	dvd_reader_t *dvdread = NULL;
    char *biggest_name = NULL;
    uint64_t biggest_size = 0;
    char *path = NULL, *dir;
    char *justname;

    if (!argname) return 0;

    path = strdup(argname);

    justname = strrchr(path, '/');

    if (!justname) { // No slash, means no dir, and no file part, just match
        justname = argname;
        dir = "/"; // from root
    } else {
        // Split into dir, and file parts.
        *justname = 0;
        justname++; // skip "/"
        dir = path; // from subdirectory
    }

    if (opt_verbose)
        printf("Looking for size of '%s' in image '%s' inside virtual directory '%s'\n",
               justname, filename, dir);

    main_setup_dvdread();
    dvdread = DVDOpen(filename);
    if (!dvdread) return 0;

    dvd_get_recurse(dvdread, dir, justname, &biggest_name, &biggest_size);

    DVDClose(dvdread);

    SAFE_FREE(path);

    if (opt_verbose)
        printf("I got biggest size %"PRIu64" and name '%s'\n",
               biggest_size,
               biggest_name ? biggest_name : "(null)");

    if (biggest_name && biggestname) {
        *biggestname = biggest_name;
        biggest_name = NULL;
    }

    SAFE_FREE(biggest_name);

    return biggest_size;
}


struct myfile_struct {
    dvd_reader_t *dvdread;
    dvd_file_t *dvdfile;
    uint64_t wanted_offset;
};
typedef struct myfile_struct myfile_t;

void *undvd_open(char *filename, char *argname)
{
	dvd_reader_t *dvdread = NULL;
    dvd_file_t *dvdfile = NULL;
    myfile_t *MYFILE;

    MYFILE = calloc(sizeof(myfile_t), 1);
    if (!MYFILE) return NULL;

    main_setup_dvdread();
    dvdread = DVDOpen(filename);
    if (!dvdread) {
        SAFE_FREE(MYFILE);
        return NULL;
    }

    dvdfile = DVDOpenFilename(dvdread, argname);
    if (!dvdfile) {
        DVDClose(dvdread);
        SAFE_FREE(MYFILE);
        return NULL;
    }

    if (opt_verbose)
        printf("Opening '%s' inside '%s'\n",
               argname, filename);

    MYFILE->dvdread = dvdread;
    MYFILE->dvdfile = dvdfile;
    return (void *)MYFILE;
}

uint64_t undvd_seek(void **iofd, MediaInfo_int64u offset, char *filename,
                    char *argname)
{
    myfile_t *MYFILE = (myfile_t *)*iofd;

    MYFILE->wanted_offset = offset;
    return offset;
}

#if (IO_SIZE < DVD_VIDEO_LB_LEN)
#error "IO_SIZE is smaller than 2048"
#endif

int undvd_read(unsigned char *buffer, int bufsize, void *iofd)
{
    myfile_t *MYFILE = (myfile_t *)iofd;
    int ret;

    if (MYFILE->wanted_offset % DVD_VIDEO_LB_LEN) {
        printf("undvd_read: uh-oh, offset %"PRIu64" is not a multiple of %u\n",
               MYFILE->wanted_offset,
               DVD_VIDEO_LB_LEN);
    }

    ret = DVDReadBlocks( MYFILE->dvdfile,
                         MYFILE->wanted_offset / (uint64_t) DVD_VIDEO_LB_LEN,
                         1, buffer);

    if (ret > 0) {
        MYFILE->wanted_offset += DVD_VIDEO_LB_LEN;
        return DVD_VIDEO_LB_LEN;
    }

    return -1;
}

int undvd_close(void *iofd)
{
    myfile_t *MYFILE = (myfile_t *)iofd;
    if (MYFILE->dvdfile)
        DVDCloseFile(MYFILE->dvdfile);
    if (MYFILE->dvdread)
        DVDClose(MYFILE->dvdread);
    SAFE_FREE(MYFILE);
    return 0;
}

uint64_t undvd_tell(void *iofd)
{
    myfile_t *MYFILE = (myfile_t *)iofd;
    return MYFILE->wanted_offset;
}
