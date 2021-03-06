#ifndef IRCD_CONFIG_H__
#define IRCD_CONFIG_H__

/* Creation time of this server */
#ifndef CREATION
#cmakedefine CREATION "@CREATION@"
#endif

/* Define to 1 if you have the setrlimit() function. */
#cmakedefine HAVE_SETRLIMIT 1

/* Define to 1 if you have the mmap() function. */
#cmakedefine HAVE_MMAP 1

/* Define to 1 if you have the mprotect() function. */
#cmakedefine HAVE_MPROTECT 1

/* Define to 1 if you have the <cygwin/in.h> header file. */
#cmakedefine HAVE_CYGWIN_IN_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine HAVE_DLFCN_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#cmakedefine HAVE_FCNTL_H 1

/* Define to 1 if you have the `getpagesize' function. */
#cmakedefine HAVE_GETPAGESIZE 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if you have the <io.h> header file. */
#cmakedefine HAVE_IO_H 1

/* Define to 1 if you have the `dl' library (-ldl). */
#cmakedefine HAVE_LIBDL 1

/* Define to 1 if you have the `duma' library (-lduma). */
#cmakedefine HAVE_LIBDUMA 1

/* Define to 1 if you have the `m' library (-lm). */
#cmakedefine HAVE_LIBM 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define to 1 if you have a working `mmap' system call. */
#cmakedefine HAVE_MMAP 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#cmakedefine HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <signal.h> header file. */
#cmakedefine HAVE_SIGNAL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#cmakedefine HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/mman.h> header file. */
#cmakedefine HAVE_SYS_MMAN_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#cmakedefine HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/resource.h> header file. */
#cmakedefine HAVE_SYS_RESOURCE_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#cmakedefine HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/timeb.h> header file. */
#cmakedefine HAVE_SYS_TIMEB_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#cmakedefine HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/wait.h> header file. */
#cmakedefine HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define to the address where bug reports for this package should be sent. */
#cmakedefine PACKAGE_BUGREPORT "@PROJECT_BUGREPORT@"

/* Define to the full name of this package. */
#define PACKAGE_NAME "@PROJECT_NAME@"

/* Define to the release name of this package. */
#define PACKAGE_RELEASE "@PROJECT_RELEASE@"

/* Define to the full name and version of this package. */
#cmakedefine PACKAGE_STRING "@PROJECT_STRING@"

/* Define to the one symbol short name of this package. */
#cmakedefine PACKAGE_TARNAME "@PROJECT_TARNAME@"

/* Define to the home page for this package. */
#cmakedefine PACKAGE_URL "@PROJECT_URL@"

/* Define to the version of this package. */
#define PACKAGE_VERSION "@PROJECT_VERSION@"

/* Platform this server runs on */
#cmakedefine PLATFORM "@PLATFORM@"

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS 1

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
#cmakedefine YYTEXT_POINTER 1

/* Define to 1 if the lexer must not include <unistd.h> */
#cmakedefine YY_NO_UNISTD_H 1
/* Define to 1 if you have the <linux/filter.h> header file. */
#cmakedefine HAVE_LINUX_FILTER_H 1

/* Define to 1 if you have the <linux/types.h> header file. */
#cmakedefine HAVE_LINUX_TYPES_H 1

/* Define to 1 if you have the <net/bpf.h> header file. */
#cmakedefine HAVE_NET_BPF_H 1

/* Define to 1 if you have the <net/ethernet.h> header file. */
#cmakedefine HAVE_NET_ETHERNET_H 1

/* Define to 1 if you have the <elf.h> header file. */
#cmakedefine HAVE_ELF_H 1

/* Define to 1 if you have the <limits.h> header file. */
#cmakedefine HAVE_LIMITS_H 1

/* Define to 1 if you have the <winsock2.h> header file. */
#cmakedefine HAVE_WINSOCK2_H 1

/* Define to 1 if you have the <ws2tcpip.h> header file. */
#cmakedefine HAVE_WS2TCPIP_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the socklen_t type. */
#cmakedefine HAVE_SOCKLEN_T 1

/* Dynamic linkable library filename extension */
#cmakedefine DLLEXT "@DLLEXT@"


#define SIZEOF_UINTPTR_T @SIZEOF_UINTPTR_T@

/* Define to the address where bug reports for this package should be sent. */
#cmakedefine PACKAGE_BUGREPORT "@PACKAGE_BUGREPORT@"

/* Define to the full name of this package. */
#cmakedefine PACKAGE_NAME "@PACKAGE_NAME@"

/* Define to the release name of this package. */
#cmakedefine PACKAGE_RELEASE "@PACKAGE_RELEASE@"

/* Define to the full name and version of this package. */
#cmakedefine PACKAGE_STRING "@PACKAGE_STRING@"

/* Define to the one symbol short name of this package. */
#cmakedefine PACKAGE_TARNAME "@PACKAGE_TARNAME@"

/* Define to the home page for this package. */
#cmakedefine PACKAGE_URL "@PACKAGE_URL@"

/* Define to the version of this package. */
#cmakedefine PACKAGE_VERSION "@PACKAGE_VERSION@"

#endif
