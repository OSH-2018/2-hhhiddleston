#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define  MAXPIPE 10

char cmd[256];
char *args[128];
int pipe_num = 0;
int arg_num = 0;
int cmd_pos[MAXPIPE];

void pipe_cmd(int i){
  if(i==pipe_num){
    if(arg_num-cmd_pos[i]>2){
      if(strcmp(args[arg_num-2], ">")==0){
        args[arg_num-2] = NULL;
        int file_d = open(args[arg_num-1], O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
        if(file_d==-1){
          printf("FILE ERROR\n");
        }
        //printf("FILENAME = %s\n", args[arg_num-1]);
        dup2(file_d, 1);
        close(file_d);
      }
      else if(strcmp(args[arg_num-2], ">>")==0){
        args[arg_num-2] = NULL;
        int file_d = open(args[arg_num-1], O_CREAT|O_RDWR|O_APPEND, S_IRUSR|S_IWUSR);
        if(file_d==-1){
          printf("FILE ERROR\n");
        }
        //printf("FILENAME = %s\n", args[arg_num-1]);
        dup2(file_d, 1);
        close(file_d);
      }
      else if(strcmp(args[arg_num-2], "<")==0){
        args[arg_num-2] = NULL;
        int file_d = open(args[arg_num-1], O_RDONLY);
        if(file_d==-1){
          printf("FILE ERROR\n");
        }
        //printf("FILENAME = %s\n", args[arg_num-1]);
        dup2(file_d, 0);
        close(file_d);
      }
    }
    execvp(args[cmd_pos[i]], args+cmd_pos[i]);
    exit(1);
  }
  int fd[2];
  pipe(fd);
  if(fork()==0){
    close(fd[0]);
    dup2(fd[1],1);
    close(fd[1]);
    execvp(args[cmd_pos[i]], args+cmd_pos[i]);
    exit(1);
  }
  close(fd[1]);
  dup2(fd[0], 0);
  close(fd[0]);
  pipe_cmd(i+1);
}

int main(){

  while(1){
    printf("# ");
    fflush(stdin);  // clean the buffer

    memset(cmd, 0, 255*sizeof(char)); // clean the cmd
    memset(args, 0, 128*sizeof(char*)); // clean the args
    memset(cmd_pos, 0, MAXPIPE*sizeof(int)); // clean cmd_pos

    fgets(cmd, 256, stdin); // read the input
    //printf("CMD %s\n", cmd);

    pipe_num = 0;
    arg_num = 0;

    int i=0,j;
    for(;cmd[i]!='\n';i++){
      ;
    }
    cmd[i]='\0'; // end the cmd
    int flag; // 0 for argument env ; 1 for space env
    args[0] = cmd;
    while(*args[0] == ' ')
      args[0]++;
    for(i=0;*args[i];i++){
      flag=0;
      for(args[i+1]=args[i]+1;*args[i+1];args[i+1]++){
        if(*args[i+1]==' '){
          flag = 1;
          *args[i+1] = '\0';
        }
        else{
          if(flag==1){
            break;
          }
        }
      }
    }
    args[i]=NULL;

    // split pipe
    cmd_pos[0] = 0;
    for(i=0;args[i]!=NULL;i++){
      arg_num++;
      //puts(args[i]);
      if(strcmp(args[i], "|") == 0){
        args[i] = NULL;
        pipe_num += 1;
        cmd_pos[pipe_num] = i+1;
      }
    }


    // no cmd
    if(!args[0])
      continue;

    // internal cmd
    if(strcmp(args[0], "cd") == 0){
      // change directory
      if(args[1])
        chdir(args[1]);
      continue;
    }
    if(strcmp(args[0], "exit") == 0){
      // exit
      return 0;
    }
    if(strcmp(args[0], "export") == 0){
      //printf("%s\n", args[1]);
      char* var_name = args[1];
      char* env_val = var_name;
      while(1){
        if(*env_val == '='){
          *env_val = '\0';
          break;
        }
        env_val++;
      }
      env_val++;

      char* val = getenv(var_name);
      if(val)
        printf("OLD: %s = %s\n", var_name, val);
      if(setenv(var_name, env_val, 1)==-1){
        printf("ERROR -- SETTING ENV.\n");
        return 255;
      }
      val = getenv(var_name);
      printf("NEW: %s = %s\n", var_name, val);
      continue;
    }
    if(strcmp(args[0], "echo")==0 && args[1][0]=='$'){
      // echo env_var
      char* var_name = args[1]+1;
      char* val = getenv(var_name);
      if(!val)
        putchar('\n');
      else
        printf("%s\n", val);
      continue;
    }

    // create pipes
    pid_t pid;
    pid = fork();
    if(pid==0){
      pipe_cmd(0);
      exit(0);
    }
    while(wait(NULL)>0);
  }
}
