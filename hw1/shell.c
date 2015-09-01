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
  }
  /** YOUR CODE HERE */
}

/**
 * Add a process to our process list
 */
void add_process(process* p)
{
  /** YOUR CODE HERE */
}

/**
 * Creates a process given the inputString from stdin
 */
process* create_process(char* inputString)
{
  /** YOUR CODE HERE */
}

void exec_process(char* inputString)
{

  char* token;
  int i = 0;
  char *tofree[4];
  char path_file[1000] = "" ;
  int pipe_to_child[2];
  int pipe_from_child[2];
  char *command ;
  char buf[256];
  char parent_buf[256];
  char* buf_pointer = malloc(sizeof(char)*(257));
  char* file = NULL;
  while ((token = strsep(&inputString, "-")) != NULL)
  {
    tofree[i]=malloc(20);
    tofree[i] = token;
    i = i + 1;
  }
  if(i>2){
    file = tofree[3];
  }
  pipe(pipe_to_child);
  pipe(pipe_from_child);
  int pID = fork();
  if (pID == 0){
    char *path = get_path_from_file(tofree[0]);
    if(i > 2){
      read(pipe_to_child[0], parent_buf, sizeof(parent_buf)); 
      printf("%s\n", parent_buf);
    }
    strcat(path, "/");
    strcat(path, tofree[0]);
    strcat(path,path_file);
    command = tofree[0];
    close(pipe_to_child[1]);
    close(pipe_from_child[0]);
    dup2(pipe_to_child[0], fileno(stdin));
    dup2(pipe_from_child[1], fileno(stdout));
    execl(path, command, tofree[1] , NULL);
    *path = NULL;
    
    exit(0);
  }
  else if(pID < 0){
    perror("Error1: ");
  }
  else {
    int returnStatus;
    if(i>2){
      write(pipe_to_child[1], tofree[3], 10);
    }
    close(pipe_to_child[0]);
    waitpid(pID, &returnStatus, 0);
    read(pipe_from_child[0], buf, sizeof(buf));
    *buf_pointer = buf;
    if(i>2){  
      FILE *fp;
      fp = fopen(file, "w");
      if (fp == NULL)
      {
        perror("Error: ") ;
        exit(1);
      }
      fprintf(fp, "%s", buf);
      fclose(fp);
    }
    else{
      printf("%s", buf);
    }
    free(buf_pointer);
  }
}

int shell (int argc, char *argv[]) {
  char *s = malloc(INPUT_STRING_SIZE+1);			/* user input string */
  tok_t *t;			/* tokens parsed from input */
  int lineNum = 0;
  int fundex = -1;
  pid_t pid = getpid();		/* get current processes PID */
  pid_t ppid = getppid();	/* get parents PID */
  pid_t cpid, tcpid, cpgid;

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
      int length_t = sizeof(t)/sizeof(t[0]);
      printf("%s %s\n",t[1], t[2]); 
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
      printf("%s \n", inputString);
      exec_process(inputString);
    }
    fprintf(stdout, "%s: ", current_directory());
  }
  return 0;
}
