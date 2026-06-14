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
    /*
     * Suporte de depuracao para a Semana 4:
     *
     * Esta funcao existe para inspecionar eventos crus depois que o runtime
     * ja consegue parar em syscalls e preencher struct syscall_event.
     * Ela nao e a formatacao final do projeto.
     *
     * Experimento sugerido:
     * - imprima o nome da syscall;
     * - imprima se o evento e entrada ou saida;
     * - imprima o pid;
     * - em eventos de entrada, observe os argumentos;
     * - em eventos de saida, observe o valor de retorno.
     *
     * Depois compare a saida de:
     *
     *   ./toytrace trace --raw-events -- ./tests/targets/hello_write
     *
     * A pergunta importante da Semana 4 e:
     * por que a mesma syscall aparece duas vezes?
     */

     /*
     * Implementação do experimento sugerido para a Semana 4:
     * O kernel gera duas paradas para uma mesma syscall. A lógica abaixo separa
     * a formatação para provar esse comportamento. Na ENTRADA (entering == 1), 
     * o kernel ainda não executou a chamada, então mostramos os registradores 
     * de argumentos. Na SAÍDA (entering == 0), a chamada terminou, então 
     * ignoramos os argumentos e mostramos apenas o valor de retorno (ret).
     */
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
