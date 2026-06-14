#include "student_api.h"

#include "syscall_names.h"

#include <stdio.h>
#include <sys/syscall.h>
#include "trace_helpers.h"

extern char saved_execve_path[256];

void student_debug_raw_event(const struct syscall_event *ev,
                             char *buf,
                             size_t bufsz)
{
    if (ev->entering) {
        snprintf(buf, bufsz, "pid=%d %s entrada (args: %#lx, %#lx, %#lx, %#lx, %#lx, %#lx)",
                 ev->pid,
                 syscall_name(ev->syscall_no),
                 ev->args[0], ev->args[1], ev->args[2],
                 ev->args[3], ev->args[4], ev->args[5]);
    } else {
        snprintf(buf, bufsz, "pid=%d %s saida (retorno: %ld)",
                 ev->pid,
                 syscall_name(ev->syscall_no),
                 ev->ret);
    }
}

void student_format_event(const struct syscall_event *ev,
                          char *buf,
                          size_t bufsz)
{
    switch (ev->syscall_no) {
        case SYS_read:
            snprintf(buf, bufsz, "read(%ld, %#lx, %lu) = %ld",
                     ev->args[0], ev->args[1], ev->args[2], ev->ret);
            break;
        case SYS_write:
            snprintf(buf, bufsz, "write(%ld, %#lx, %lu) = %ld",
                     ev->args[0], ev->args[1], ev->args[2], ev->ret);
            break;
        case SYS_openat: {
            char path_buf[256];
            if (read_child_string(ev->pid, ev->args[1], path_buf, sizeof(path_buf)) == 0) {
                snprintf(buf, bufsz, "openat(%ld, \"%s\", %#lx, %#lx) = %ld",
                         ev->args[0], path_buf, ev->args[2], ev->args[3], ev->ret);
            } else {
                snprintf(buf, bufsz, "openat(%ld, \"<ilegivel>\", %#lx, %#lx) = %ld",
                         ev->args[0], ev->args[2], ev->args[3], ev->ret);
            }
            break;
        }
        case SYS_execve: {
            if (saved_execve_path[0] != '\0') {
                snprintf(buf, bufsz, "execve(\"%s\", ...) = %ld", saved_execve_path, ev->ret);
            } else {
                snprintf(buf, bufsz, "execve(\"<ilegivel>\", ...) = %ld", ev->ret);
            }
            saved_execve_path[0] = '\0'; // Limpa para o proximo uso
            break;
        }
        case SYS_exit_group:
            snprintf(buf, bufsz, "exit_group(%ld) = %ld", ev->args[0], ev->ret);
            break;
        default:
            snprintf(buf, bufsz, "%s(%#lx, %#lx, %#lx, %#lx, %#lx, %#lx) = %ld",
                     syscall_name(ev->syscall_no),
                     ev->args[0], ev->args[1], ev->args[2],
                     ev->args[3], ev->args[4], ev->args[5],
                     ev->ret);
            break;
    }
}
