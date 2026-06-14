#include "trace_runtime.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#if !defined(__x86_64__)
#error "Este runtime didatico suporta apenas Linux x86_64."
#endif

static void fill_event_from_regs(pid_t pid,
                                 int entering,
                                 const struct user_regs_struct *regs,
                                 struct syscall_event *ev)
{

    memset(ev, 0, sizeof(*ev));
    ev->pid = pid;
    ev->entering = entering;
    
    ev->syscall_no = regs->orig_rax;
    ev->ret = regs->rax;
    
    ev->args[0] = regs->rdi;
    ev->args[1] = regs->rsi;
    ev->args[2] = regs->rdx;
    ev->args[3] = regs->r10;
    ev->args[4] = regs->r8;
    ev->args[5] = regs->r9;
}

static pid_t launch_tracee(char *const argv[])
{

    pid_t pid = fork();
    //Verificação de erro na criação do processo filho
    if (pid < 0) {
        perror("erro ao criar processo com fork");
        return -1;
    }

    // Fluxo do filho
    if (pid == 0) {
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
            perror("erro no ptrace(PTRACE_TRACEME)");
            exit(EXIT_FAILURE); 
        }

        // Para a própria execução para o pai configurar o trace
        if (raise(SIGSTOP) != 0) {
            perror("erro ao enviar SIGSTOP");
            exit(EXIT_FAILURE);
        }

        execvp(argv[0], argv);

        /* execvp só retorna se der erro */
        perror("erro no execvp");
        exit(EXIT_FAILURE);
    }

    // Fluxo do pai, só retorna o pid do filho
    return pid;
}

static int wait_for_initial_stop(pid_t child)
{

    int status;

    //O pai espera por qualquer mudança de estado do processo filho
    if (waitpid(child, &status, 0) < 0) {
        perror("erro no waitpid");
        return -1;
    }

    // Verifica se o motivo do retorno do waitpid foi uma parada (SIGSTOP)
    if (WIFSTOPPED(status)) {
        return 0; // o filho parou como esperado antes do execvp
    }
    
    fprintf(stderr, "erro: processo filho nao parou no SIGSTOP inicial\n");
    return -1;
}

static int configure_trace_options(pid_t child)
{

    if (ptrace(PTRACE_SETOPTIONS, child, NULL, PTRACE_O_TRACESYSGOOD) < 0) {
        perror("erro no ptrace(PTRACE_SETOPTIONS)");
        return -1;
    }
    
    return 0;
}

static int resume_until_next_syscall(pid_t child, int signal_to_deliver)
{

    // faz o filho rodar até entrar ou sair de uma syscall
    if (ptrace(PTRACE_SYSCALL, child, NULL, signal_to_deliver) < 0) {
        perror("erro no ptrace(PTRACE_SYSCALL)");
        return -1;
    }
    
    return 0;
}

static int wait_for_syscall_stop(pid_t child, int *status)
{

    while (1) {
        if (waitpid(child, status, 0) < 0) {
            perror("erro no waitpid");
            return -1;
        }

        if (WIFEXITED(*status) || WIFSIGNALED(*status)) {
            return 0;
        }

        if (WIFSTOPPED(*status)) {
            int sig = WSTOPSIG(*status);

            if (sig & 0x80) {
                return 1;
            }

            // Se não for syscall, precisamos mandar o processo continuar.
            // A especificação diz que paradas comuns de SIGTRAP não devem ser repassadas, mas 
            // outros sinais sim.
            int signal_to_deliver = 0;
            if (sig != SIGTRAP) {
                signal_to_deliver = sig;
            }

            if (ptrace(PTRACE_SYSCALL, child, NULL, signal_to_deliver) < 0) {
                perror("erro no ptrace(PTRACE_SYSCALL) ignorando sinal");
                return -1;
            }
        }
    }
}

int trace_program(char *const argv[],
                  trace_observer_fn observer,
                  void *userdata)
{
    pid_t child;
    int status = 0;
    int entering = 1;

    if (argv == NULL || argv[0] == NULL) {
        fprintf(stderr, "erro: programa alvo ausente\n");
        return -1;
    }

    child = launch_tracee(argv); //função que cria o processo alvo e armazena o pid em child
    if (child < 0) {
        return -1;
    }

    if (wait_for_initial_stop(child) < 0) {
        return -1;
    }

    if (configure_trace_options(child) < 0) {
        return -1;
    }

    if (resume_until_next_syscall(child, 0) < 0) {
        return -1;
    }

    while (1) {
        struct user_regs_struct regs;
        struct syscall_event ev;
        int stop_kind;

        stop_kind = wait_for_syscall_stop(child, &status);
        if (stop_kind < 0) {
            return -1;
        }
        if (stop_kind == 0) {
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
            if (WIFSIGNALED(status)) {
                return 128 + WTERMSIG(status);
            }
            return 0;
        }


        memset(&regs, 0, sizeof(regs));
        
        if (ptrace(PTRACE_GETREGS, child, NULL, &regs) < 0) {
            perror("erro no ptrace(PTRACE_GETREGS)");
            return -1;
        }

        fill_event_from_regs(child, entering, &regs, &ev);
        
        if (observer != NULL) {
            observer(&ev, userdata);
        }

        entering = !entering;

        if (resume_until_next_syscall(child, 0) < 0) {
            return -1;
        }
    }
}
