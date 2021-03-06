#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <fcntl.h>
#define FALSE 0
#define TRUE 1
#define INPUT_STRING_SIZE 80

#include "io.h"
#include "parse.h"
#include "process.h"
#include "shell.h"

int cmd_quit(tok_t arg[]) {
  printf("Bye\n");
  exit(0);
  return 1;
}

int cmd_help(tok_t arg[]);
int cmd_change_directory(tok_t arg[]);

/* Command Lookup table */
typedef int cmd_fun_t (tok_t args[]); /* cmd functions take token array and return int */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_quit, "quit", "quit the command shell"},
  {cmd_change_directory, "cd", "change the directory"},
};

char *get_path_from_file(char *file)
{
  char* token;
  char *path = getenv("PATH");
  struct dirent *ent;
  while ((token = strsep(&path, ":")) != NULL)
  {
    DIR *dir;
    if ((dir = opendir (token)) != NULL) {
      while ((ent = readdir (dir)) != NULL)
      {
        int rc = strcmp(ent->d_name, file);
        if(rc == 0)
        {
          return token;
          break;
        }
      }
      closedir(dir);
    }
  }
  return NULL;
}

char * current_directory()
{
  char cwd[1024];
  if(getcwd(cwd, sizeof(cwd)) != NULL)
    return cwd;
  else
    return "Current directory not found!";
}

int cmd_change_directory(tok_t arg[]) {
  int ret;
  ret = chdir(*arg);
   if(ret!=0){
     perror("Error: ");
   }
  return 0;
}

int cmd_help(tok_t arg[]) {
  int i;
  for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
    printf("%s - %s\n",cmd_table[i].cmd, cmd_table[i].doc);
  }
  return 1;
}

int lookup(char cmd[]) {
  int i;
  for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0)) return i;
  }
  return -1;
}

void init_shell()
{
  /* Check if we are running interactively */
  shell_terminal = STDIN_FILENO;

  /** Note that we cannot take control of the terminal if the shell
      is not interactive */
  shell_is_interactive = isatty(shell_terminal);

  if(shell_is_interactive){
     struct sigaction new_action, old_action;  
     signal(SIGINT, SIG_IGN);
     signal(SIGTSTP, SIG_IGN);
    /* force into foreground */
    while(tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp()))
      kill( - shell_pgid, SIGTTIN);

    shell_pgid = getpid();
    /* Put shell in its own process group */
    if(setpgid(shell_pgid, shell_pgid) < 0){
      perror("Couldn't put the shell in its own process group");
      exit(1);
    }
    
    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);
    tcgetattr(shell_terminal, &shell_tmodes);

    first_process = (process *)malloc(sizeof(process));
    first_process->pid = getpid();
    first_process->stopped = 0;
    first_process->completed = 0;
    first_process->background = 0;
    //tcgetattr(shell_terminal, &first_process->tmodes);
    first_process->stdin = STDIN_FILENO;
    first_process->stdout = STDOUT_FILENO;
    first_process->stderr = STDERR_FILENO;
    first_process->prev = first_process->next = NULL;
  }
  /** YOUR CODE HERE */
}

/**
 * Add a process to our process list
 */
void add_process(process* p)
{
  /** YOUR CODE HERE */
  process *q = first_process;
  while (q->next) {
    q=q->next;
  }
  q->next = p;
  p->prev = q;

}

/**
 * Creates a process given the inputString from stdin
 */
process* create_process(tok_t *t)
{
  /** YOUR CODE HERE */
  process* procInfo = (process *)malloc(sizeof(process));
  int i = 0;
  int total_toks = 0;

  if (t == NULL || t[0] == NULL || strlen(t[0]) == 0)
    return NULL;

  procInfo->stdin = STDIN_FILENO;
  procInfo->stdout = STDOUT_FILENO;
  procInfo->stderr = STDERR_FILENO;
  procInfo->argv = t;
  total_toks = totalToks(t);
  procInfo->completed = 0;
  procInfo->stopped = 0;
  procInfo->background = 0;
  procInfo->status = 0;
  //procInfo->tmodes = ;
  procInfo->next = NULL;
  procInfo->prev = NULL;
  if(strcmp(t[1], "<") == 0)
  {
    FILE *inputFile;
    if ((inputFile = fopen(t[3], "r")) != NULL) {
      procInfo->stdin = fileno(inputFile);
    }
  }
  else if(t[2] != NULL && strcmp(t[2], ">") == 0)
  {
    FILE *outputFile;
    printf("%s\n", t[3]);
    if ((outputFile = fopen(t[3], "w")) != NULL) {
      procInfo->stdout = fileno(outputFile);
    }
  }

  else if(t[total_toks - 1] == '&'){
     procInfo->background = 1;
  }
  return procInfo;
}

void update_status (void) {
  int status;
  pid_t pid;

  do {
    pid = waitpid(WAIT_ANY, &status, WUNTRACED|WNOHANG);
  } while (!mark_process_status(pid, status));
}

void exec_process(char* inputString, process* proc)
{
  char* token;
  int i = 0;
  char *tofree[4];
  char path_file[1000] = "" ;
  char *command ;
  char buf[256];
  char parent_buf[256];
  int returnStatus;
  char* buf_pointer = malloc(sizeof(char)*(257));
  char* file = NULL;
  while ((token = strsep(&inputString, "-")) != NULL)
  {
    tofree[i]=malloc(20);
    tofree[i] = token;
    i = i + 1;
  }
  char *path = get_path_from_file(tofree[0]);
  pid_t pid, pID;
  strcat(path, "/");
  strcat(path, tofree[0]);
  strcat(path,path_file);
  command = tofree[0];
  dup2(proc->stdin, STDIN_FILENO);
  dup2(proc->stdout, STDOUT_FILENO);
  execl(path, command, tofree[1] , NULL);
}

int shell (int argc, char *argv[]) {
  char *s = malloc(INPUT_STRING_SIZE+1);			/* user input string */
  tok_t *t;			/* tokens parsed from input */
  int lineNum = 0;
  int fundex = -1;
  pid_t pid = getpid();		/* get current processes PID */
  pid_t ppid = getppid();	/* get parents PID */
  pid_t cpid, tcpid, cpgid;
  char *status;
  char* proc = "/proc/";
  char str_pid[10];
  snprintf(str_pid, 10, "%d", ppid);
  char* out = (char*)malloc(strlen(proc) + strlen(str_pid) + 1);
  strcpy(out, proc);
  strcat(out, str_pid);
  strcat(out, "/status");
  struct stat sts;
  char str[80] ;
  init_shell();

  printf("%s running as PID %d under %d\n",argv[0],pid,ppid);

  lineNum=0;
  char cwd[1024];
  char* inputString;
  fprintf(stdout, "%s: ", current_directory());
  while ((s = freadln(stdin))){
    t = getToks(s); /* break the line into tokens */
    fundex = lookup(t[0]); /* Is first token a shell literal */
    if(fundex >= 0) cmd_table[fundex].fun(&t[1]);
    else {
      process *p = create_process(t);
      if (p != NULL) {
        add_process(p);
        pid_t pid = fork();
         if(t[2] != NULL && strcmp(t[2], ">") == 0)
         {
           inputString = malloc(strlen(t[0]) + strlen(t[1]) + strlen(t[2]) + strlen(t[3]) + 1); 
           strcpy(inputString, t[0]);
           strcat(inputString, "-");
           strcat(inputString, t[1]);
           strcat(inputString, "-");
           strcat(inputString, t[2]);
           strcat(inputString, "-");
           strcat(inputString, t[3]);
        } 
        else if(strcmp(t[1], "<") == 0)
        {
          inputString = malloc(strlen(t[0]) + strlen(t[1]) + strlen(t[2]) + 1); 
          strcpy(inputString, t[0]);
          strcat(inputString, "-");
          strcat(inputString, t[2]);
        }
        else 
        {
          inputString = malloc(strlen(t[0]) + strlen(t[1]) + 1); 
          strcpy(inputString, t[0]);
          strcat(inputString, "-");
          strcat(inputString, t[1]);
        }
        if (pid > 0) {  /* parent process */
          p->pid = pid;
          setpgid(pid, pid);
          printf("Run process: %s on pid: %d %d\n", t[0], p->pid, p->background);
          if (!p->background) {
            put_process_in_foreground(p, 0);
          }
          printf("Executed\n");
        }
        else if (pid == 0) {  /* child process */
          if (p != NULL) {
            p->pid = getpid();
            exec_process(inputString, p);
          }
          //exec_process(inputString, p);
        }
      } //printf("executed\n");
    }
    update_status();
    fprintf(stdout, "%s: ", current_directory());
  }
  return 0;
}
