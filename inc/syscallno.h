#ifndef INC_SYSCALLNO_H
#define INC_SYSCALLNO_H

#define MAXARG       32  /* max exec arguments */
#define NR_SYSCALL   256
/* #define SYS_exec 0
#define SYS_exit 1

#define SYS_dup 23
#define SYS_mknodat 33
#define SYS_mkdirat 34
#define SYS_chdir 49
#define SYS_openat 56
#define SYS_close 57
#define SYS_read 63
#define SYS_write 64
#define SYS_writev 66
#define SYS_newfstatat 79
#define SYS_fstat 80
#define SYS_execve 221
 */
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))

#endif