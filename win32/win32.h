// Misc WIN32 types and defines needed.
typedef unsigned __int64 uint64_t;
#define PRIu64 "I64d"
#define strdup _strdup
#define strtoull(X,Y,Z) _atoi64(X)
#define snprintf _snprintf
#define fileno _fileno
#define fseek _fseeki64
#define strcasecmp _stricmp