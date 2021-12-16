
#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <sys/stat.h>
#include <errno.h>

#ifdef WIN32
#define _WINDOWS_MEAN_AND_LEAN
#include <Windows.h>
#include "win32.h"
#endif


#if MEDIAINFO_STATIC
#include <MediaInfoDLL/MediaInfoDLL_Static.h>
#else
#include <MediaInfoDLL/MediaInfoDLL.h>
#endif


#include "lfnmatch.h"
#include "my_popen.h"
#include "unrar.h"
#include "undvd.h"
#include "misc.h"
#include "rarinput.h"

#include "dvdread/dvd_reader.h"

typedef struct dvd_input_s *dvd_input_t;

//
// Attempt to replicate the output of MediaInfo commandline tool, but
// add the ability to handle RAR archives.
//
// Jorgen Lundman <lundman@lundman.net>
#include <getopt.h>

extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

int opt_full    = 0;
int opt_xml     = 0;
int opt_verbose = 0;
int opt_list    = 0;
int opt_size    = 0;

static char *rar_insidename = NULL;

static const struct option longopts[] = {
    {"Full",    no_argument,        NULL, 'f' },
    {"help",    no_argument,        NULL, 'h' },
    {"Verbose", no_argument,        NULL, 'v' },
    {"Output",  required_argument,  NULL, 'O' },
    {"List",    no_argument,        NULL, 'l' },
    {"Size",    no_argument,        NULL, 's' },
    {NULL,                0,        NULL, '\0' }
};


int arguments(int argc, char **argv);
void usage(int style);


int myInform(char *filename, char *argname, int type);



// Alas, dvd_input.h is not installed by default.
int dvdinput_setup_ext(
                       dvd_input_t (*dvdinput_open)  (const char *),
                       int         (*dvdinput_read)  (dvd_input_t, void *, int, int),
                       int         (*dvdinput_seek)  (dvd_input_t, int),
                       int         (*dvdinput_close) (dvd_input_t),
                       int         (*dvdinput_title) (dvd_input_t, int),
                       char *      (*dvdinput_error) (dvd_input_t)
                       );



int main(int argc, char **argv, char **envp)
{
    int result = 0;
    char *r;
    char *path = NULL;

    if (!arguments(argc, argv))
        exit(1);

    argc -= optind;
    argv += optind;

    if (!argc) {
        usage(1);
        return 0;
    }

    // Make a copy of the filename so we can modify it.
    path = strdup(argv[0]);

    // Let's also support the case of "dir/file.rar/file.avi"
    // strstr is terrible though..
    // We also need to check for the case of
    // dir/file.rar/file.iso/file"
    if ((r = strstr(path, ".rar/")) ||
        (r = strstr(path, ".RAR/")) ||
        (r = strstr(path, ".001/"))) {

        r[4] = 0;
        rar_insidename = &r[5];

        // NO-iso, so just regular RAR
        if (!(r = strstr(rar_insidename, ".iso/")) &&
            lfnmatch("*.iso", rar_insidename, LFNM_CASEFOLD)) {
            result = myInform(path, rar_insidename, IS_RAR);
            SAFE_FREE(path);
            return 0;
        }
    }

    // If "file.rar/file.iso" then rar_insidename is set to "file.iso/..."
    if ((r = strstr(rar_insidename ? rar_insidename : path,
                    ".iso/"))) {
        char *smallx;

        smallx = strdup(&r[4]);
        r[4] = 0;

        result = myInform(path, smallx, IS_ISO);

        SAFE_FREE(path);
        SAFE_FREE(smallx);
		return 0;

    }



    if (!lfnmatch("*.rar", argv[0], LFNM_CASEFOLD) ||
        !lfnmatch("*.001", argv[0], LFNM_CASEFOLD)) {

        result = myInform(path, argv[1], IS_RAR);
        SAFE_FREE(path);
		return 0;
    }

    if (!lfnmatch("*.iso", argv[0], LFNM_CASEFOLD)) {
        result = myInform(path, argv[1], IS_ISO);
        SAFE_FREE(path);
		return 0;
    }


    result = myInform(argv[0], NULL, NOT_RAR);
    SAFE_FREE(path);
    return result;

}



void main_setup_dvdread(void)
{
    if (rar_insidename) {
        dvdinput_setup_ext(rarinput_open,
                           rarinput_read,
                           rarinput_seek,
                           rarinput_close,
                           rarinput_title,
                           rarinput_error);
    } else {
        dvdinput_setup_ext(NULL, NULL, NULL, NULL, NULL, NULL);
    }
}

char *main_get_insidename(void)
{

    if (rar_insidename) return strdup(rar_insidename);
    return NULL;
}


int myInform(char *filename, char *argname, int type)
{
    unsigned char buffer[IO_SIZE];
    int result = 1;  // Assume failure
    char *inform_str = NULL;
    void *handle = NULL;
    uint64_t fsize = 0;
    FILE *iofd = NULL;
    FILE *isofd = NULL;
    int bread = 0;
    MediaInfo_int64u diff = 0;
    int pos = 0;
    MediaInfo_int64u wanted_offset = -1;
    MediaInfo_int64u unrar_offset = 0;
    char *firstname = NULL;
    struct stat stbf;
    int Status = 0;

    if (opt_verbose)
        printf("Processing type %d with filename '%s', argname '%s' and inside_rar '%s', opt_list %d\n",
               type,
               filename,
               argname,
               rar_insidename,
               opt_list);

    if (type == IS_RAR) {

        if (opt_list) {
            unrar_list(filename, argname);
            result = 0;
            goto failed;
        }

        fsize = unrar_getsize(filename,
                              argname ? argname : "*",
                              &firstname);

        if (!fsize) {
            printf("Unable to get filesize of file '%s' in archive '%s'\n",
                   argname ? argname : "*", filename);
            goto failed;
        }

    }

    if (type == IS_ISO) {

        if (opt_list) {
            undvd_list(filename, argname);
            result = 0;
            goto failed;
        }

        fsize = undvd_getsize(filename,
                              argname ? argname : "*.m2ts",
                              &firstname);
        if (!fsize)
            fsize = undvd_getsize(filename,
                                  argname ? argname : "*",
                                  &firstname);

        if (!fsize) {
            printf("Unable to get image file '%s' in archive '%s'\n",
                   argname ? argname : "*", filename);
            goto failed;
        }

    }

    if (opt_size && firstname) {
        printf("%12"PRIu64" %s\n",
               fsize, firstname);
        return 0;
    }



    // firstname is only used with RARs
    if (type == IS_ISO) {

        isofd = undvd_open(filename, firstname);
        if (!isofd) {
            perror("open:");
            goto failed;
        }

    } else {

        iofd = unrar_open(filename, firstname, type);
        if (!iofd) {
            perror("open:");
            goto failed;
        }

        if (type == NOT_RAR) {
            if (!fstat(fileno(iofd), &stbf))
                fsize = (uint64_t) stbf.st_size;
        }
    }


    if (!fsize) {
        printf("Unable to get filesize of file '%s'\n",
               filename);
        goto failed;
    }

#if !MEDIAINFO_STATIC
	if (MediaInfoDLL_Load() != 1) goto failed;
#endif

	// Get MediaInfo ready...
    handle=MediaInfo_New();

    if (opt_verbose)
        printf("MediaInfo said %p\n", handle);

    if (!handle) goto failed;

    //inform_str = (char *)MediaInfo_Option(handle, "Internet", "No");
    //printf("MediaInfo_Option said %s\n", inform_str);

    // Open first argument given
    //result = MediaInfo_Open(handle, argv[1]);
    result = MediaInfo_Open_Buffer_Init(handle, fsize, 0);

    if (opt_verbose)
        printf("MediaInfo_Open_Buffer_Init() returns %d\n", result);

    if (!result) goto failed;

    do {

        // Read more data, or data already in buffer?
        if (wanted_offset == (MediaInfo_int64u) -1) {

            //Reading data somewhere, do what you want for this.
            if (type == IS_ISO)
                bread=undvd_read(buffer, sizeof(buffer), isofd);
            else
                bread=unrar_read(buffer, sizeof(buffer), iofd, type);

            if (bread <= 0) break;

            //Sending the buffer to MediaInfo
            Status=MediaInfo_Open_Buffer_Continue(handle, buffer, bread);

        } else {

            // Data already in buffer
            MediaInfo_Open_Buffer_Init(handle, fsize, wanted_offset);

            if (type == IS_ISO)
                unrar_offset = undvd_tell(isofd);
            else
                unrar_offset = unrar_tell();

            diff = unrar_offset - wanted_offset;
            pos = (int) ( bread - diff);
            bread = bread - pos;

            //Sending the buffer to MediaInfo
            Status=MediaInfo_Open_Buffer_Continue(handle,
                                                      &buffer[pos],
                                                      bread);
#ifdef VERBOSE
            printf("Inside buffer: pos is %d and new bread %d\n",
                   pos, bread);
#endif
        }

        if (Status&0x08) //Bit3=Finished
            break;

        //Testing if there is a MediaInfo request to go elsewhere
        wanted_offset = MediaInfo_Open_Buffer_Continue_GoTo_Get(handle);
        if (wanted_offset != (MediaInfo_int64u) -1 ) {

            // Sometimes we read a whole buffer, say 8192, but MediaInfo
            // want to seek forward only a few hundred bytes, still inside
            // buffer, so seeking is not required. Note, unrar_offset is
            // incremented in unrar_read, so already increased here.
            if (type == IS_ISO)
                unrar_offset = undvd_tell(isofd);
            else
                unrar_offset = unrar_tell();

            if ((wanted_offset >= (unrar_offset - bread)) &&
                (wanted_offset < (unrar_offset))) {
                // Woah, still in our buffer!
#ifdef VERBOSE
                printf("Seek still inside buffer!\n");
#endif
                // Leave wanted_offset here!
                continue;
            }


            // Seek
            if (type == IS_ISO) {
                if (undvd_seek(&isofd, wanted_offset,
                               filename, firstname)) //Position the file
                    break; // Failed
            } else {
                if (unrar_seek(&iofd, wanted_offset,
                               filename, firstname, type)) //Position the file
                    break; // Failed
            }

            //Informing MediaInfo we have seeked
            MediaInfo_Open_Buffer_Init(handle, fsize, wanted_offset);
            wanted_offset =  (MediaInfo_int64u) -1;
        }
 	}
 	while (bread>0);

 	//Finalizing
 	MediaInfo_Open_Buffer_Finalize(handle);

    //MediaInfo_Menu_Debug_Complete(handle, 1);

    // -f --full
    if (opt_full)
#ifdef UNICODE
        MediaInfo_Option(handle, _T("Complete"), _T("1"));
#else
		MediaInfo_Option(handle, "Complete", "1");
#endif

    if (opt_xml)
#ifdef UNICODE
		MediaInfo_Option(handle, _T("Inform"), _T("XML"));
#else
		MediaInfo_Option(handle, "Inform", "XML");
#endif

    inform_str = (char *)MediaInfo_Inform(handle, 0);



    // MediaInfo uses window's style "\r" only, so change them to \n for unix.
#ifndef WIN32
    if (inform_str && *inform_str) {
        char *r;
        r = inform_str;
        while(*r && (r = strchr(inform_str, '\r'))) *r = '\n';
    }
#endif



    if (opt_xml) {
#ifdef UNICODE
        char *r;
        int distance;
        r = (char *)wcsstr((wchar_t *)inform_str, _T("<File>"));
        if (r) {
            distance = &r[6] - inform_str;
            wprintf(_T("%*.*s"), distance, distance, inform_str);

            if (type != NOT_RAR) {
                printf("\n<Filename>%s</Filename>\n", filename);
                printf("<Media_name>%s</Media_name>\n", firstname);
                printf("<llink_URL>%s?f=%"PRIu64",%s</llink_URL>\n",
                       filename, fsize, firstname);
                printf("<File_size>%"PRIu64"</File_size>\n",
                       fsize);

            }

            wprintf(_T("%s\n"), &((wchar_t *)r)[6]);
        } else {
            wprintf(_T("%s"), inform_str);
		}
#else
        char *r;
        int distance;
        r = strstr(inform_str, "<File>");
        if (r) {
            distance = &r[6] - inform_str;
            printf("%*.*s", distance, distance, inform_str);

            if (type != NOT_RAR) {
                printf("\n<Filename>%s</Filename>\n", filename);
                printf("<Media_name>%s</Media_name>\n", firstname);
                printf("<llink_URL>%s?f=%"PRIu64",%s</llink_URL>\n",
                       filename, fsize, firstname);
                printf("<File_size>%"PRIu64"</File_size>\n",
                       fsize);

            }

            printf("%s\n", &r[6]);
        }
#endif


    } else { // Not XML



        if (type != NOT_RAR) {
            printf("Filename                         : %s\n", filename);
            printf("Media name                       : %s\n", firstname);
            printf("llink URL                        : %s?f=%"PRIu64",%s\n",
                   filename, fsize, firstname);
            printf("File size                        : %"PRIu64"\n",
                   fsize);

        }

#ifndef UNICODE
        printf("\n%s\n\n", inform_str);
#else
        wprintf(_T("\n%s\n\n"), inform_str);
#endif

    } // XML

    // Return GOOD
    result = 0;

 failed:

    if (result)
        printf("MediaInfo::Inform failed. (Possibly: %s)\n",
               strerror(errno));

    if (handle)
        MediaInfo_Delete(handle);

    if (iofd)
        unrar_close(iofd, type);

    if (isofd)
        undvd_close(isofd);

    if (firstname)
        SAFE_FREE(firstname);

    return result;

}



void usage(int style)
{

    if (style) {
        printf("Usage: \"MediaInfo [-Options...] FileName1 [Filename2...]\"\n");
        printf("\"MediaInfo --Help\" for displaying more information\n");
        return;
    }

    printf(
"MediaInfo Command line,\n"
"MediaInfoLib - v0.7.29\n"
"Usage: \"MediaInfo [-Options...] FileName1 [Filename2...]\"\n"
"\n"
"Options:\n"
"--Help, -h         Display this help and exit\n"
"--Help-Inform      Display help for Inform= option [X]\n"
"--Help-AnOption    Display help for \"AnOption\" [X]\n"
"--Version          Display MediaInfo version and exit\n"
"\n"
"--Full , -f        Full information Display (all internal tags)\n"
"--Output=HTML      Full information Display with HTML tags [X]\n"
"--Output=XML       Full information Display with XML tags \n"
"--Inform=...       Template defined information Display [X]\n"
"--Info-Parameters  Display list of Inform= parameters [X]\n"
"\n"
"--Language=raw     Display non-translated unique identifiers (internal text) [X]\n"
"--LogFile=...      Save the output in the specified file\n"
"--list , -l        List contents of ISO files\n"
"--size , -s        Show 'largest file' automatic selection only\n");
    printf("--Verbose , -v     Verbose information\n");
    printf("\n[X] Not actually implemented. Bug developer for it\n\n");

    printf("mediainfo-rar %s by Jorgen Lundman <lundman@lundman.net>\n",
           VERSION);
}

int arguments(int argc, char **argv)
{
    int opt;
    int option_index = 0;

    while ((opt=getopt_long(argc, argv,
                       "hfvls"
#ifdef WIN32
                       ""
#endif
                            , longopts, &option_index
                       )) != -1) {

        switch(opt) {

        case 0: // long option
            break;

        case 'h':
            usage(0);
            return 0;
            break;

        case 'f':
            opt_full ^= 1;
            break;

        case 'v':
            opt_verbose ^= 1;
            break;

        case 'l':
            opt_list ^= 1;
            break;

        case 's':
            opt_size ^= 1;
            break;

        case 'O':
            if (optarg &&
                !strcasecmp("XML", optarg)) opt_xml = 1;
            break;

        default:
            usage(0);
            return 0;
            break;
        }
    }

    return 1;
}
