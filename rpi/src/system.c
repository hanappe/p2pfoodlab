#define _POSIX_SOURCE
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include "system.h"
#include "log_message.h"

process_t* new_process(pid_t id,
                       int in,
                       int out,
                       int err)
{
        process_t* p = (process_t*) malloc(sizeof(process_t));
        if (p == NULL) {
                log_err("System: new_process: out of memory");
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
                log_err("System: process_wait failed: %s", strerror(errno));
                
        } else if (WIFEXITED(p->status)) {
                p->exited = 1;
                p->ret = WEXITSTATUS(p->status);

        } else if (WIFSIGNALED(p->status)) {
                log_err("System: process_wait failed: child process received a signal");

        } else if (WIFSTOPPED(p->status)) {
                log_err("System: process_wait failed: child process received a stop signal");
        } else {
                log_err("System: process_wait failed");
        }
}

static int _process_kill(process_t* p, int sig, const char* signame)
{
        int ret = kill(p->id, SIGINT);

        if (ret == 0)  {
                pid_t id = waitpid(p->id, &p->status, WNOHANG);

                if (id == -1) {
                        log_err("System: process_kill: waitpid failed: %s", strerror(errno));
                        return -1;
                        
                } else if (WIFEXITED(p->status)) {
                        p->exited = 1;
                        p->ret = WEXITSTATUS(p->status);
                        return 0;
                }

        } else if (errno == ESRCH) {
                p->exited = 1;
                p->ret = 0; // FIXME...
                return 0;

        } else {
                log_err("System: process_kill: kill(%s) failed: %s", 
                        signame, strerror(errno));
                return -1;
        }
        return -1;
}

int process_kill(process_t* p)
{
        int r = _process_kill(p, SIGINT, "SIGINT");
        if (r != 0) {
                sleep(5);
                r = _process_kill(p, SIGKILL, "SIGKILL");
        }
        return r;
}
	
process_t* system_exec(char* const argv[], int wait)
{
        int r;
        int in[2];
        int out[2];
        int err[2];

        if (pipe(in) != 0) {
                log_err("System: pipe failed: %s", strerror(errno));
                return NULL;
        }

        if (pipe(out) != 0) {
                log_err("System: pipe failed: %s", strerror(errno));
                return NULL;
        }

        if (pipe(err) != 0) {
                log_err("System: pipe failed: %s", strerror(errno));
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
                log_err("System: fork failed: %s", strerror(errno));
                return NULL;

        } else if (id == 0) {
                // Child
                close(in[1]);
                close(out[0]);
                close(err[0]);

                r = dup2(in[0], STDIN_FILENO);
                if (r == -1) {
                        log_err("System: dup2 failed: %s", strerror(errno));
                        exit(1);
                }

                r = dup2(out[1], STDOUT_FILENO);
                if (r == -1) {
                        log_err("System: dup2 failed: %s", strerror(errno));
                        exit(1);
                }

                r = dup2(err[1], STDERR_FILENO);
                if (r == -1) {
                        log_err("System: dup2 failed: %s", strerror(errno));
                        exit(1);
                }

                close(in[0]);
                close(out[1]);
                close(err[1]);

                r = execv(argv[0], argv);

                log_err("System: execv failed: %s", strerror(errno));
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

int system_run(char* const argv[], int timeout)
{
        if (1) {
                char buffer[1024];
                int buflen = 0;
                buffer[0] = 0;

                for (int i = 0; argv[i] != NULL; i++) {
                        int arglen = strlen(argv[i]);
                        if (buflen + arglen + 2 < sizeof(buffer)) {                        
                                strcat(buffer, argv[i]);
                                strcat(buffer, " ");
                        }
                        buflen += arglen + 1;
                }
                buffer[sizeof(buffer)-1] = 0;

                log_info("System: Running %s", buffer);
        }

        process_t* p = system_exec(argv, 0);
        if (p == NULL) 
                return -1;

        fd_set rfds;
        struct timeval tv;
        int retval;
        time_t start_time = time(NULL);
        time_t duration = 0;

        while (1) {

                FD_ZERO(&rfds);
                FD_SET(p->out, &rfds);
                FD_SET(p->err, &rfds);

                tv.tv_sec = 1;
                tv.tv_usec = 0;

                int nfds = (p->out > p->err)? p->out : p->err;

                retval = select(nfds + 1, &rfds, NULL, NULL, &tv);

                if (retval == -1) {
                        log_err("System: select() failed", strerror(errno));
                        break;

                } else if (retval) {
                        char buffer[1024];
                        int count = 0;

                        if (FD_ISSET(p->out, &rfds)) {
                                ssize_t n = read(p->out, buffer, 1023);
                                if (n > 0) {
                                        count++;
                                        buffer[n] = 0;
                                        log_info("%s", buffer);
                                }
                        }
                        if (FD_ISSET(p->err, &rfds)) {
                                ssize_t n = read(p->err, buffer, 1023);
                                if (n > 0) {
                                        count++;
                                        buffer[n] = 0;
                                        log_err("%s", buffer);
                                } 
                        }
                        if (count == 0)
                                break;

                }

                duration = time(NULL) - start_time;
                if ((timeout > 0) && (duration > timeout)) {
                        log_err("System: %s: timed out", argv[0]);
                        break;
                }
        }

        duration = time(NULL) - start_time;
        if ((timeout > 0) && (duration > timeout))
                process_kill(p);
        else 
                process_wait(p);

        int ret = (p->exited && (p->ret == 0))? 0 : -1;

        delete_process(p);

        return ret;
}

int system_get_serial_number(char *buffer, int len)
{
        FILE *f = fopen("/proc/cpuinfo", "r");
        if (f == NULL)
                return -1;
   
        char line[256];
        int ret = -1;
        while (fgets(line, 256, f)) {
                if (strncmp(line, "Serial", 6) == 0) {
                        strncpy(buffer, strchr(line, ':') + 2, len);
                        buffer[len-1] = 0;
                        ret = 0;
                        break;
                }
        }
        fclose(f);

        return ret;
}
