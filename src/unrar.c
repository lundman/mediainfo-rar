#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

#ifdef WIN32
#include "win32.h"
#endif

#include "my_popen.h"
#include "unrar.h"
#include "lfnmatch.h"
#include "misc.h"
#include "undvd.h"


extern int opt_verbose;


/*
 *
 * unrar.c actually handled both plain POSIX file IO as well as the unrar
 * pipe IO. So that the caller only needs to call one set of functions.
 *
 */

// This means only one unrar file at a time, really, the unrar_* methods should
// return a void *, and this variable is part of that...
static uint64_t unrar_offset = 0;


char *unrar_path(void)
{
    static char *unrar_path = NULL;
    static int firsttime = 1;

    // Attempt to guess where unrar executable might be, if unknown, use the
    // compiled default
    if (firsttime) {
        firsttime = 0;

        unrar_path = getenv("MEDIAINFO_UNRAR");

    }

    if (!unrar_path)
        return UNRAR_CMD;

    return unrar_path;

}




int unrar_parse(char *argname, char *line, uint64_t *result, char **firstname)
{
    static int parse = 0;          // Hold parsing state
    static char *rar_name = NULL;  // Hold name between alternating lines
    char *fsize, *packed, *ratio, *date, *thyme, *attr;
    char *ar;
    int directory;

    if (!line) {
        parse = 0;
        SAFE_FREE(rar_name);
        return 0;
    }

    // Kill newlines
    if ((ar = strchr(line, '\r')))
        *ar = 0;
    if ((ar = strchr(line, '\n')))
        *ar = 0;

    switch(parse) {
    case 0:
        // start of unrar list, waiting for line with -----
        if (!strncmp("--------------------", line, 20))
            parse++;
        break;

    case 1:
        // woohooo, actually parsing entries
        // This is the filename part
        // " directory1/directory2/file2.avi"

        // Have we finished output?
        if (!strncmp("--------------------", line, 20)) {
            parse = 0;
            break;
        }

        // unrar filename line starts with a single space.
        if (*line != ' ') {
            printf("unable to parse: '%s'\n", line);
            break;
        }

        // Skip that leading space.
        line++;

        // Remember this line until next parsing line.
        SAFE_FREE(rar_name);
        rar_name = strdup(line);
        parse++; // Go to parsing next line
        break;

    case 2:
        // parse out filesize, type etc.
        // "   7       17 242% 04-12-07 14:11 -rw-r--r-- 16B28489 m3b 2.9"

        // Alternate filename/data lines, change back to filename
        parse = 1;

        ar = line;

        // node->rar_name hold the FULL path name.
        fsize  = misc_digtoken(&ar, " \r\n");
        packed = misc_digtoken(&ar, " \r\n");
        ratio  = misc_digtoken(&ar, " \r\n");
        date   = misc_digtoken(&ar, " \r\n");
        thyme  = misc_digtoken(&ar, " \r\n");
        attr   = misc_digtoken(&ar, " \r\n");

        if (!rar_name ||
            !fsize|| !*fsize||
            !attr || !*attr) {
            printf("unable to parse -- skipping\n");
            SAFE_FREE(rar_name);
            break;
        }

        // Files spanning Volumes will be repeated, but we can tell by
        // lookip at "ratio" column.
        if (!strcmp("<->", ratio) ||
            !strcmp("<--", ratio)) {

            SAFE_FREE(rar_name);
            break;
        }


        // Now we need to look at the rar_file fullname, and
        // split it into directory/filename
        // Find last slash. Will windows use '\' ?
#ifdef WIN32
		{
			char *name;
			for (name = rar_name; *name; name++)
				if (*name == '\\') *name = '/';
		}
#endif

        directory = 0;
        if ((tolower(*attr) == 'd') ||
            (tolower(attr[1]) == 'd'))
            directory = 1;

        // Test if the rar name is the same as argname, then return size.
        // Also handle case where argname is NULL, then return first found
        // file.
        if (!lfnmatch(argname, rar_name, LFNM_CASEFOLD)) {

            // drw-r--r--         1392 VIDEO_TS
            // -rw-r--r--        16384 VIDEO_TS/VIDEO_TS.IFO
            if (!result && !firstname) {
                printf("%crw-r--r-- %12s %s\n",
                       directory ? 'd' : '-',
                       fsize,
                       rar_name);
            }

            // Return the first found file? Remember the name, skip the free
            if (firstname) {
                uint64_t size;

                size = strtoull(fsize, NULL, 10);

                if(size > *result) {
                    SAFE_FREE(*firstname);
                    *firstname = rar_name; // No strdup, set to NULL below
                    *result = size;
                    rar_name = NULL; // Set to NULL, to not free here.
                }
            }
        }

        // Release name
        SAFE_FREE(rar_name);

        break;

    case 3:  // seen ----- the rest is fluff
        break;


    } // switch parse

    return 0;

}


int unrar_list(char *filename, char *argname)
{
    char buffer[8192];
    FILE *iofd;
    uint64_t result = 0;

    // List the RAR
    snprintf(buffer, sizeof(buffer), "%s v -v -c- -p- -y -cfg- -- \"%s\"",
             unrar_path(),
             filename);

    iofd = my_popen(buffer);
    if (!iofd) return 0;

    // reset internal state
    unrar_parse(NULL, NULL, NULL, NULL);

    while(fgets(buffer, sizeof(buffer), iofd)) {

        unrar_parse(argname?argname:"*", buffer, NULL, NULL);

    }
    my_pclose(iofd);

    return 0;
}


uint64_t unrar_getsize(char *filename, char *argname, char **firstname)
{
    char buffer[8192];
    FILE *iofd;
    uint64_t result = 0;

    // List the RAR, and get the filesize of "argname"
    snprintf(buffer, sizeof(buffer), "%s v -v -c- -p- -y -cfg- -- \"%s\"",
             unrar_path(),
             filename);

    iofd = my_popen(buffer);
    if (!iofd) return 0;

    // reset internal state
    unrar_parse(NULL, NULL, NULL, NULL);

    while(fgets(buffer, sizeof(buffer), iofd)) {

        unrar_parse(argname, buffer, &result, firstname);

    }
    my_pclose(iofd);

    if (opt_verbose)
        printf("I got size %lu and firstname '%s'\n",
               (unsigned long)result,
               firstname && *firstname ? *firstname : "(null)");

    return result;
}


FILE *unrar_open(char *filename, char *argname, int type)
{
    FILE *result = NULL;
    char buffer[8192];

    unrar_offset = 0;

    if (type == NOT_RAR) {

        result = fopen(filename, "rb");
        if (!result) return NULL;

        return result;
    }

    // RAR type

    // Spawn unrar with starting at 0.
    snprintf(buffer, sizeof(buffer),
             "%s p -inul -c- -p- -y -cfg- -- \"%s\" \"%s\"",
             unrar_path(),
             filename,
             argname);

    if (opt_verbose)
        printf("running: %s\n", buffer);

    result = my_popen(buffer);

    return result;
}


int unrar_read(unsigned char *buffer, int bufsize, FILE *iofd, int type)
{
    int bytes;


    // RAR type
    bytes = fread(buffer, 1, bufsize, iofd);

#ifdef VERBOSE
    printf("read %lu starting at %lu: %02X %02X %02X\n",
           bytes,
           unrar_offset,
           buffer[0], buffer[1], buffer[2]);
#endif

    if (bytes > 0)
        unrar_offset += bytes;

    return bytes;
}


int unrar_seek(FILE **iofd, MediaInfo_int64u offset, char *filename,
               char *argname, int type)
{
    char buffer[8192];

    if (!iofd) return -1;

    if (type == NOT_RAR)
        return fseek(*iofd, offset, SEEK_SET);


    // RAR type
#ifdef VERBOSE
    printf("seek to %lu (current %lu)\n",
           (unsigned long) offset,
           (unsigned long) unrar_offset);
#endif

    // Perfect match?
    if (offset == unrar_offset) return 0;


    // If we only need a small seek forward, we could eat bytes...
    if ((offset > unrar_offset) &&
        ((offset - unrar_offset) <= MAXIMUM_SEEK)) {

#ifdef VERBOSE
        printf("Attempting small seek forward...\n");
#endif

        while(offset > unrar_offset) {
            MediaInfo_int64u read_size;

            // How much to read?
            read_size = offset - unrar_offset;

            // but no bigger than our buffer eh
            if (read_size > sizeof(buffer))
                read_size = sizeof(buffer);

            if ((fread(buffer, (int)read_size, 1, *iofd)) != 1) {
                /* Something failed, lets close, and let's try respawn */
                my_pclose(*iofd);
                *iofd = NULL;
                break;
            }

            // Increase out offset counter
#ifdef VERBOSE
            printf("read %lu - from %lu next start at %lu\n",
                   read_size,
                   (unsigned long)unrar_offset,
                   (unsigned long)unrar_offset + read_size);
#endif

            unrar_offset += read_size;

        } // while a little behind
    } // if only small read

    // If pipe is still ok, and we are at the right place, return OK
    if (*iofd && (offset == unrar_offset)) return 0;

    // Otherwise, if seek is in reverse, or bigger than MAXIMUM_SEEK
    // respawn process
    // If seek is in reverse, or "too big", we need to restart process.
#ifdef VERBOSE
    printf("Seek is in reverse, or too large - respawning\n");
#endif
    unrar_offset = 0;
    my_pclose(*iofd);
    *iofd = NULL;

    // Spawn unrar with starting at 0.
    snprintf(buffer, sizeof(buffer),
             "%s p -inul -c- -p- -y -cfg- -sk%"PRIu64" -- \"%s\" \"%s\"",
             unrar_path(),
             offset,
             filename,
             argname);

    if (opt_verbose)
        printf("running: %s\n", buffer);

    *iofd = my_popen(buffer);

    // Since we spawned exactly were we want to be, set offset counter
    unrar_offset = offset;

    if (*iofd) return 0;

    return -1;
}


int unrar_close(FILE *iofd, int type)
{

    unrar_offset = 0;

    if (type == NOT_RAR)
        return fclose(iofd);

    // RAR type
    return my_pclose(iofd);

}

uint64_t unrar_tell(void)
{
    return unrar_offset;
}

