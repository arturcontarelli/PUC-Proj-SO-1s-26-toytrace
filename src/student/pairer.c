#include "student_api.h"
#include "trace_helpers.h"
#include <sys/syscall.h>

char saved_execve_path[256] = "";

int student_pair_syscall(struct syscall_pairer *pairer,
                         const struct syscall_event *ev,
                         struct syscall_event *out)
{
    if (ev->entering) {
        if (pairer->has_entry) {
            return -1; // Dessincronização
        }

        if (ev->syscall_no == SYS_execve) {
            if (read_child_string(ev->pid, ev->args[0], saved_execve_path, sizeof(saved_execve_path)) < 0) {
                saved_execve_path[0] = '\0';
            }
        }

        pairer->entry = *ev;
        pairer->has_entry = 1;
        return 0; // Evento incompleto
    } else {
        if (!pairer->has_entry) {
            return -1; // Dessincronização
        }
        *out = pairer->entry;
        out->ret = ev->ret;
        out->entering = 0;
        pairer->has_entry = 0;
        return 1; // Syscall completa
    }
}
