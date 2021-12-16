
uint64_t undvd_getsize(char *filename, char *argname, char **firstname);
int undvd_list(char *filename, char *argname);
void *undvd_open(char *filename, char *argname);
uint64_t undvd_seek(void **iofd, MediaInfo_int64u offset, char *filename,
                    char *argname);
int undvd_read(unsigned char *buffer, int bufsize, void *iofd);
int undvd_close(void *iofd);
uint64_t undvd_tell(void *iofd);

char *main_get_insidename(void);
void main_setup_dvdread(void);


char *unrar_path(void);
