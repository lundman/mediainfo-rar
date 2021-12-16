
/*
 * Input IO handler for libdvdread, to read DVDs directly from RAR archives
 *
 * Uses API call dvdinput_setup_ext() to setup function pointers.
 *
 * - lundman
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <ctype.h>

#include <inttypes.h>
#ifdef WIN32
#include <io.h>
#include "win32.h"
#endif

#if MEDIAINFO_STATIC
#include <MediaInfoDLL/MediaInfoDLL_Static.h>
#else
#include <MediaInfoDLL/MediaInfoDLL.h>
#endif

#include <errno.h>
#include <fcntl.h>

#include <dvdread/dvd_reader.h>

#include "rarinput.h"
#include "undvd.h"
#include "my_popen.h"


/**
 * initialize and open a DVD device or file.
 */
dvd_rinput_t rarinput_open(const char *target)
{
  dvd_rinput_t dev;
#ifdef DEBUG
  int ret;
  char buffer[1024];
#endif

  /* Allocate the library structure */
  dev = (dvd_rinput_t) malloc(sizeof(*dev));
  if(dev == NULL) {
    fprintf(stderr, "libdvdread: Could not allocate memory.\n");
    return NULL;
  }

  dev->unrar_stream = NULL;
  dev->unrar_file   = NULL;
  dev->seek_pos     = 0;  /* Assume start of file */
  dev->current_pos  = 0;

  // We also need the name inside the RAR archive, passed along in
  // global variables. Shudder. All not to change libdvdread API.
  dev->unrar_file  = strdup(target);
  dev->unrar_archive = main_get_insidename();

  /* Lets call unrar, to list the release, as a way to quickly check that
	 all the volumes are present, and the unrar_cmd works. */
#ifdef DEBUGX

  if (dvdinput_firstrun) {

	  dvdinput_firstrun = 0;

	  snprintf(buffer, sizeof(buffer), "%s v -v -c- -p- -y -cfg- -- \"%s\"",
			   unrar_path(), dev->unrar_archive);

	  dev->unrar_stream = my_popen(buffer);

	  if(!dev->unrar_stream) {
		  perror("libdvdread: Could not spawn unrar: ");
		  fprintf(stderr, "cmd: '%s'\r\n", buffer);
		  free(dev);
		  return NULL;
	  }

	  while(fgets(buffer, sizeof(buffer), dev->unrar_stream)) ;

	  ret = my_pclose(dev->unrar_stream);
	  dev->unrar_stream = NULL;

	  /* Only allow returncode 0? Or also allow "missing archives" for those
		 who want to start watching while it is still downloading? */
	  fprintf(stderr, "libdvdread: unrar test complete: %d\r\n",
			  WEXITSTATUS(ret));

	//	  if (WIFEXITED(ret) && (WEXITSTATUS(ret) == 0)) {
	  if (WEXITSTATUS(ret) == 0) {
		  return dev;
	  }

	  perror("libdvdread: Could not test rar archive: ");
	  free(dev);
	  return NULL;

  } /* firstrun */

#endif


  return dev;
}


/**
 * return the last error message
 */
char *rarinput_error(dvd_rinput_t dev)
{
  /* use strerror(errno)? */
  return (char *)"unknown error";
}

/**
 * seek into the device.
 */
int rarinput_seek(dvd_rinput_t dev, int blocks)
{
#ifdef DEBUGX
  fprintf(stderr, "libdvdread: rarfile_seek(block -> %d)\r\n", blocks);
#endif

  dev->seek_pos = (off_t)blocks * (off_t)DVD_VIDEO_LB_LEN;

  /* assert pos % DVD_VIDEO_LB_LEN == 0 */
  return (int) (dev->seek_pos / DVD_VIDEO_LB_LEN);
}

/**
 * set the block for the begining of a new title (key).
 */
int rarinput_title(dvd_rinput_t dev, int block)
{
  return -1;
}

/**
 * read data from the device.
 *
 * Unfortunately, the UDF layer asks for the same block MANY times, and
 * we have to "seek back" one block by restarting unrar. This should be
 * fixed. Please do so if you have time.
 *
 */

//
// Without one buffer cache, listing ISO in RAR takes:
// real    1m18.690s
// Enabling one buffer cache (saving just one block)
// real    0m4.155s
//
// I think we'll keep it!

#define ONE_BUFFER_CACHE

int rarinput_read(dvd_rinput_t dev, void *buffer, int blocks, int flags)
{
  size_t len, read_size;
  ssize_t ret;
  char ibuffer[DVD_VIDEO_LB_LEN];
#ifdef ONE_BUFFER_CACHE
  static char one_buffer_cache[DVD_VIDEO_LB_LEN];
  static uint64_t one_buffer_offset = 1; // Non block alignment, wont match
#endif

#ifdef DEBUGX
  fprintf(stderr, "libdvdread: rarfile_read (%d blocks)\r\n", blocks);
#endif

#ifdef ONE_BUFFER_CACHE
  // If they want to be exactly where we already are, and ask for
  // exactly one block, return the cache!
  if ((dev->seek_pos == one_buffer_offset) &&
      (blocks == 1)) {
      memcpy(buffer, one_buffer_cache, DVD_VIDEO_LB_LEN);
      // Pretend we read a block.
      dev->seek_pos += DVD_VIDEO_LB_LEN;
      return blocks;
  }
#endif

  /* First, position ourselves where the API wants us. This may mean
   * spawning new unrar, or possibly eating data until the correct
   * position. */

  /* We already have a stream, here, we can be exactly at the right place,
   * or, need to eat data until the correct position, or
   * if the seek is too far, close this unrar, and spawn a new.
   */
  if (dev->unrar_stream) {

	  /* eat data? */
	  if (dev->seek_pos > dev->current_pos) {

		  /* Seek too far? Better to spawn new unrar? */
		  if ((dev->seek_pos - dev->current_pos) >=
              (off_t)MAXIMUM_SEEK_SIZE) {
#ifdef DEBUG
			  fprintf(stderr, "libdvdread: seek too large, re-spawning unrar\r\n");
#endif
			  my_pclose(dev->unrar_stream);
			  dev->unrar_stream = NULL;

		  } else {

			  /* Not too far, read and eat bytes. */
			  while (dev->seek_pos > dev->current_pos) {

				  /* Work out how much we need to read, but no larger than
				   * the size of our buffer.*/

				  read_size = dev->seek_pos - dev->current_pos;
				  if (read_size > sizeof(ibuffer))
					  read_size = sizeof(ibuffer);

				  if ((fread(ibuffer, read_size, 1, dev->unrar_stream)) != 1) {
					  /* Something failed, lets close, and let's try respawn */
					  my_pclose(dev->unrar_stream);
					  dev->unrar_stream = NULL;
					  break;
				  }

				  dev->current_pos += read_size;
			  } /* while seek > current */
		  } /* NOT >= max seek */
	  } /* NOT seek > current */

	  /* Also check if seek < current, then we must restart unrar */
	  if (dev->seek_pos < dev->current_pos) {

		  my_pclose(dev->unrar_stream);
		  dev->unrar_stream = NULL;

	  }

  } /* we have active stream */

  /* Spawn new unrar? */
  if (!dev->unrar_stream) {
 	  snprintf(ibuffer, sizeof(ibuffer),
			   "%s p -inul -c- -p- -y -cfg- -sk%"PRIu64" -- \"%s\" \"%s\"",
			   unrar_path(),
			   dev->seek_pos,
			   dev->unrar_file,
			   dev->unrar_archive);

#ifdef DEBUGX
	  fprintf(stderr, "libdvdvread: spawning '%s' for %"PRIu64"\r\n",
              ibuffer, dev->seek_pos);
#endif

#ifndef WIN32
      // We must make sure PIPE is set to terminate process when we close
      // the handle to the unrar child
      {
          static struct sigaction sa, restore_pipe_sa;
          sa.sa_flags = 0;
          sigemptyset (&sa.sa_mask);
          sa.sa_handler = SIG_DFL;
          sigaction (SIGPIPE, &sa, &restore_pipe_sa);
#endif

          dev->unrar_stream = my_popen(ibuffer);

#ifndef WIN32
          // Restore signal
          sigaction(SIGPIPE, &restore_pipe_sa, NULL);
      }
#endif

	  if (!dev->unrar_stream) {
		  return -1;
	  }

	  /* Update ptr */
	  dev->current_pos = dev->seek_pos;

  }

  /* Assert current == seek ? */
#ifdef DEBUG
  if (dev->current_pos != dev->seek_pos)
	  fprintf(stderr, "libdvdread: somehow, current_pos != seek_pos!?\r\n");
#endif

  len = (size_t)blocks * DVD_VIDEO_LB_LEN;

  while(len > 0) {

	  /* ret = read(dev->fd, buffer, len); */
	  ret = fread(buffer, 1, len, dev->unrar_stream);

#ifdef DEBUGX
	  fprintf(stderr, "libdvdread: fread(%d) -> %d\r\n", (int)len, (int)ret);
#endif

	  if((ret != len) && ferror(dev->unrar_stream)) {
		  /* One of the reads failed, too bad.  We won't even bother
		   * returning the reads that went ok, and as in the posix spec
		   * the file postition is left unspecified after a failure. */
		  return -1;
	  }

	  if (ret < 0)
		  return -1;

	  if ((ret != len) || feof(dev->unrar_stream)) {
		  /* Nothing more to read.  Return the whole blocks, if any,
		   * that we got. and adjust the file position back to the
		   * previous block boundary. */
		  size_t bytes = (size_t)blocks * DVD_VIDEO_LB_LEN - len; /* 'ret'? */
		  off_t over_read = -(bytes % DVD_VIDEO_LB_LEN);
		  /*off_t pos =*/ /*lseek(dev->fd, over_read, SEEK_CUR);*/
		  dev->current_pos += (off_t)len;
		  dev->seek_pos = dev->current_pos + (off_t)len + over_read;
		  /* minus over_read, I did not touch the code above, but I wonder
		   * if it is correct. It does not even use "ret" in the math. */

		  /* should have pos % 2048 == 0 */
		  return (int) (bytes / DVD_VIDEO_LB_LEN);
	  }

#ifdef ONE_BUFFER_CACHE
      // Save in one buffer cache
      if (blocks == 1) {
          memcpy(one_buffer_cache, buffer, DVD_VIDEO_LB_LEN);
          one_buffer_offset = dev->current_pos;
      }
#endif

	  dev->current_pos += (off_t) ret;
	  dev->seek_pos += (off_t) ret;
	  len -= ret;
  }

  return blocks;
}

/**
 * close the DVD device and clean up.
 */
int rarinput_close(dvd_rinput_t dev)
{

#ifdef DEBUG
    fprintf(stderr, "libdvdread: rarfile_close\r\n");
#endif

  if (dev->unrar_stream)
	  my_pclose(dev->unrar_stream);
  dev->unrar_stream = NULL;

  if (dev->unrar_archive)
	  free(dev->unrar_archive);
  dev->unrar_archive = NULL;

  if (dev->unrar_file)
	  free(dev->unrar_file);
  dev->unrar_file = NULL;

  free(dev);

  return 0;
}



