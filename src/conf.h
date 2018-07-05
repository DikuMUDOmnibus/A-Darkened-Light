/* src/conf.h.  Generated automatically by configure.  */
/* src/conf.h.in.  Generated automatically from cnf/configure.in by autoheader.  */
#ifndef __CONF_H__
#define __CONF_H__
/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if we're compiling CircleMUD under any type of UNIX system */
#define CIRCLE_UNIX 1

/* Define if the system is capable of using crypt() to encrypt */
#define CIRCLE_CRYPT 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef ssize_t */

/* Define if you have the inet_addr function.  */
#define HAVE_INET_ADDR 1

/* Define if you have the inet_aton function.  */
#define HAVE_INET_ATON 1

/* Define if you have the <arpa/inet.h> header file.  */
#define HAVE_ARPA_INET_H 1

/* Define if you have the <arpa/telnet.h> header file.  */
#define HAVE_ARPA_TELNET_H 1

/* Define if you have the <assert.h> header file.  */
#define HAVE_ASSERT_H 1

/* Define if you have the <crypt.h> header file.  */
#define HAVE_CRYPT_H 1

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <net/errno.h> header file.  */
/* #undef HAVE_NET_ERRNO_H */

/* Define if you have the <netdb.h> header file.  */
#define HAVE_NETDB_H 1

/* Define if you have the <netinet/in.h> header file.  */
#define HAVE_NETINET_IN_H 1

/* Define if you have the <signal.h> header file.  */
#define HAVE_SIGNAL_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <strings.h> header file.  */
#define HAVE_STRINGS_H 1

/* Define if you have the <sys/fcntl.h> header file.  */
#define HAVE_SYS_FCNTL_H 1

/* Define if you have the <sys/resource.h> header file.  */
#define HAVE_SYS_RESOURCE_H 1

/* Define if you have the <sys/select.h> header file.  */
#define HAVE_SYS_SELECT_H 1

/* Define if you have the <sys/socket.h> header file.  */
#define HAVE_SYS_SOCKET_H 1

/* Define if you have the <sys/stat.h> header file.  */
#define HAVE_SYS_STAT_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/types.h> header file.  */
#define HAVE_SYS_TYPES_H 1

/* Define if you have the <sys/uio.h> header file.  */
#define HAVE_SYS_UIO_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the malloc library (-lmalloc).  */
/* #undef HAVE_LIBMALLOC */

/* Define if your compiler does not prototype accept().  */
/* #undef NEED_ACCEPT_PROTO */

/* Define if your compiler does not prototype atoi().  */
/* #undef NEED_ATOI_PROTO */

/* Define if your compiler does not prototype atol().  */
/* #undef NEED_ATOL_PROTO */

/* Define if your compiler does not prototype bind().  */
/* #undef NEED_BIND_PROTO */

/* Define if your compiler does not prototype bzero().  */
/* #undef NEED_BZERO_PROTO */

/* Define if your compiler does not prototype chdir().  */
/* #undef NEED_CHDIR_PROTO */

/* Define if your compiler does not prototype close().  */
/* #undef NEED_CLOSE_PROTO */

/* Define if your compiler does not prototype crypt().  */
/* #undef NEED_CRYPT_PROTO */

/* Define if your compiler does not prototype fclose().  */
/* #undef NEED_FCLOSE_PROTO */

/* Define if your compiler does not prototype fcntl().  */
/* #undef NEED_FCNTL_PROTO */

/* Define if your compiler does not prototype fflush().  */
/* #undef NEED_FFLUSH_PROTO */

/* Define if your compiler does not prototype fprintf().  */
/* #undef NEED_FPRINTF_PROTO */

/* Define if your compiler does not prototype fputc().  */
/* #undef NEED_FPUTC_PROTO */

/* Define if your compiler does not prototype fputs().  */
/* #undef NEED_FPUTS_PROTO */

/* Define if your compiler does not prototype fread().  */
/* #undef NEED_FREAD_PROTO */

/* Define if your compiler does not prototype fscanf().  */
/* #undef NEED_FSCANF_PROTO */

/* Define if your compiler does not prototype fseek().  */
/* #undef NEED_FSEEK_PROTO */

/* Define if your compiler does not prototype fwrite().  */
/* #undef NEED_FWRITE_PROTO */

/* Define if your compiler does not prototype getpeername().  */
/* #undef NEED_GETPEERNAME_PROTO */

/* Define if your compiler does not prototype getpid().  */
/* #undef NEED_GETPID_PROTO */

/* Define if your compiler does not prototype getrlimit().  */
/* #undef NEED_GETRLIMIT_PROTO */

/* Define if your compiler does not prototype getsockname().  */
/* #undef NEED_GETSOCKNAME_PROTO */

/* Define if your compiler does not prototype gettimeofday().  */
/* #undef NEED_GETTIMEOFDAY_PROTO */

/* Define if your compiler does not prototype htonl().  */
/* #undef NEED_HTONL_PROTO */

/* Define if your compiler does not prototype htons().  */
/* #undef NEED_HTONS_PROTO */

/* Define if your compiler does not prototype inet_addr().  */
/* #undef NEED_INET_ADDR_PROTO */

/* Define if your compiler does not prototype inet_aton().  */
/* #undef NEED_INET_ATON_PROTO */

/* Define if your compiler does not prototype inet_ntoa().  */
/* #undef NEED_INET_NTOA_PROTO */

/* Define if your compiler does not prototype listen().  */
/* #undef NEED_LISTEN_PROTO */

/* Define if your compiler does not prototype ntohl().  */
/* #undef NEED_NTOHL_PROTO */

/* Define if your compiler does not prototype perror().  */
/* #undef NEED_PERROR_PROTO */

/* Define if your compiler does not prototype printf().  */
/* #undef NEED_PRINTF_PROTO */

/* Define if your compiler does not prototype qsort().  */
/* #undef NEED_QSORT_PROTO */

/* Define if your compiler does not prototype read().  */
/* #undef NEED_READ_PROTO */

/* Define if your compiler does not prototype rewind().  */
/* #undef NEED_REWIND_PROTO */

/* Define if your compiler does not prototype select().  */
/* #undef NEED_SELECT_PROTO */

/* Define if your compiler does not prototype setitimer().  */
/* #undef NEED_SETITIMER_PROTO */

/* Define if your compiler does not prototype setrlimit().  */
/* #undef NEED_SETRLIMIT_PROTO */

/* Define if your compiler does not prototype setsockopt().  */
/* #undef NEED_SETSOCKOPT_PROTO */

/* Define if your compiler does not prototype socket().  */
/* #undef NEED_SOCKET_PROTO */

/* Define if your compiler does not prototype sprintf().  */
/* #undef NEED_SPRINTF_PROTO */

/* Define if your compiler does not prototype sscanf().  */
/* #undef NEED_SSCANF_PROTO */

/* Define if your compiler does not prototype system().  */
/* #undef NEED_SYSTEM_PROTO */

/* Define if your compiler does not prototype time().  */
/* #undef NEED_TIME_PROTO */

/* Define if your compiler does not prototype unlink().  */
/* #undef NEED_UNLINK_PROTO */

/* Define if your compiler does not prototype write().  */
/* #undef NEED_WRITE_PROTO */

#endif
