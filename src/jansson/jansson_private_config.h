#if defined(NV_LINUX)
# define HAVE_ENDIAN_H 1
#endif

/* Prevent value.c from overriding Solaris's builtin isnan() */
#if defined(NV_SUNOS)
# define isnan isnan
#endif

#define HAVE_FCNTL_H 1
#define HAVE_SCHED_H 1
#define HAVE_UNISTD_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_STDINT_H 1

#define HAVE_CLOSE 1
#define HAVE_GETPID 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_OPEN 1
#define HAVE_READ 1
#define HAVE_SCHED_YIELD 1

/* #undef HAVE_SYNC_BUILTINS */
/* #undef HAVE_ATOMIC_BUILTINS */

#define HAVE_LOCALE_H 1
#define HAVE_SETLOCALE 1

#define HAVE_INT32_T 1
#ifndef HAVE_INT32_T
#  define int32_t int32_t
#endif

#define HAVE_UINT32_T 1
#ifndef HAVE_UINT32_T
#  define uint32_t uint32_t
#endif

#define HAVE_SSIZE_T 1

#ifndef HAVE_SSIZE_T
#  define ssize_t 
#endif

#define HAVE_SNPRINTF 1

#ifndef HAVE_SNPRINTF
#  define snprintf snprintf
#endif

/* #undef HAVE_VSNPRINTF */

#define USE_URANDOM 1
#define USE_WINDOWS_CRYPTOAPI 1
