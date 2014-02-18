#define _POSIX_SOURCE
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include "system.h"
#include "log_message.h"

process_t* new_process(pid_t id,
                       int in,
                       int out,
                       int err)
{
        process_t* p = (process_t*) malloc(sizeof(process_t));
        if (p == NULL) {
                log_err("Network: new_process: out of memory");
                return NULL;
        }
        p->id = id;
        p->in = in;
        p->out = out;
        p->err = err;
        p->status = -1;
        p->exited = 0;
        p->ret = -1;
        return p;
}

void delete_process(process_t* p)
{
        if (p) {
                kill(p->id, SIGINT);
                close(p->in);
                close(p->out);
                close(p->err);
                free(p);
        }
}

void process_wait(process_t* p)
{
        pid_t id = waitpid(p->id, &p->status, WUNTRACED);

        if (id == -1) {
                log_err("Network: process_wait failed: %s", strerror(errno));
                
        } else if (WIFEXITED(p->status)) {
                p->exited = 1;
                p->ret = WEXITSTATUS(p->status);

        } else if (WIFSIGNALED(p->status)) {
                log_err("Network: process_wait failed: child process received a signal");

        } else if (WIFSTOPPED(p->status)) {
                log_err("Network: process_wait failed: child process received a stop signal");
        } else {
                log_err("Network: process_wait failed");
        }
}
	
process_t* system_exec(const char* path, char* const argv[], int wait)
{
        int r;
        int in[2];
        int out[2];
        int err[2];

        if (pipe(in) != 0) {
                log_err("Network: pipe failed: %s", strerror(errno));
                return NULL;
        }

        if (pipe(out) != 0) {
                log_err("Network: pipe failed: %s", strerror(errno));
                return NULL;
        }

        if (pipe(err) != 0) {
                log_err("Network: pipe failed: %s", strerror(errno));
                return NULL;
        }

        pid_t id = fork();
      
        if (id == -1) {
                close(in[0]);
                close(in[1]);
                close(out[0]);
                close(out[1]);
                close(err[0]);
                close(err[1]);
                log_err("Network: fork failed: %s", strerror(errno));
                return NULL;

        } else if (id == 0) {
                // Child
                close(in[1]);
                close(out[0]);
                close(err[0]);

                r = dup2(in[0], STDIN_FILENO);
                if (r == -1) {
                        log_err("Network: dup2 failed: %s", strerror(errno));
                        exit(1);
                }

                r = dup2(out[1], STDOUT_FILENO);
                if (r == -1) {
                        log_err("Network: dup2 failed: %s", strerror(errno));
                        exit(1);
                }

                r = dup2(err[1], STDERR_FILENO);
                if (r == -1) {
                        log_err("Network: dup2 failed: %s", strerror(errno));
                        exit(1);
                }

                close(in[0]);
                close(out[1]);
                close(err[1]);

                r = execv(path, argv);

                log_err("Network: execv failed: %s", strerror(errno));
                exit(1);			  

        } else {

                // Parent			
                close(in[0]);
                close(out[1]);
                close(err[1]);

                process_t* p = new_process(id, in[1], out[0], err[0]);
                if (p == NULL)
                        return NULL;

                if (wait) {
                        process_wait(p);
                }

                return p;
        }
}
