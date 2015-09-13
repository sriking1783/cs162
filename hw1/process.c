#include "process.h"
#include "shell.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <termios.h>

/**
 * Executes the process p.
 * If the shell is in interactive mode and the process is a foreground process,
 * then p should take control of the terminal.
 */
void launch_process(char* inputString, process* proc)
{
  /** YOUR CODE HERE */

}


int mark_process_status (pid_t pid, int status)
{
  process *p;
  if (pid > 0){
    for (p = first_process; p; p = p->next)
      if(p->pid == pid){
        p->status = status;
        if(WIFSTOPPED(status))
          p->stopped = 1;
        else{
          p->completed = 1;
          if (WIFSIGNALED (status))
            fprintf (stderr, "%d: Terminated by signal %d.\n",
                     (int) pid, WTERMSIG (p->status));
                }
        return 0;
      }
    fprintf (stderr, "No child process %d.\n", pid);
    return -1;
    
  }
  return -1; 
}

void put_process_in_foreground (process *p, int cont)
{
  int status;
  /* Put the job into the foreground.  */
  tcsetpgrp (shell_terminal, p->pid);
  /* Send the job a continue signal, if necessary.  */
  if (cont)
    {
      tcsetattr (shell_terminal, TCSADRAIN, &p->tmodes);
      if (kill (- p->pid, SIGCONT) < 0)
        perror ("kill (SIGCONT)");
    }
  /* Wait for it to report.  */
   waitpid(WAIT_ANY, &status, WUNTRACED);

  /* Put the shell back in the foreground.  */
  tcsetpgrp (shell_terminal, shell_pgid);

  /* Restore the shell’s terminal modes.  */
  tcgetattr (shell_terminal, &p->tmodes);
  tcsetattr (shell_terminal, TCSADRAIN, &shell_tmodes);
}


void put_process_in_background (process *p, int cont)
{
  /* Send the job a continue signal, if necessary.  */
  if (cont)
    if (kill (-p->pid, SIGCONT) < 0)
      perror ("kill (SIGCONT)");
}
