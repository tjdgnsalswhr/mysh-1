#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#define SOCK_PATH "tpf_unix_sock.server"


#include "commands.h"
#include "built_in.h"



static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};

static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;
    }
  }

  return -1; // Not found
}

/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  if (n_commands == 1)
  {
    struct single_command* com = (*commands);

    assert(com->argc != 0);

    int built_in_pos = is_built_in_command(com->argv[0]);
    if (built_in_pos != -1)
    {
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv))
      {
        if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0)
        {
          fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
        }
      }
      else
      {
        fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
        return -1;
      }
    }
    else if (strcmp(com->argv[0], "") == 0)
    {
      return 0;
    }
    else if (strcmp(com->argv[0], "exit") == 0) 
    {
      return 1;
    }
    else
    {
      int pid;
      pid = fork();
      if(pid==0)
      {
         if(execv(com->argv[0], com->argv)==-1)
         {
           fprintf(stderr, "%s: command not found\n", com->argv[0]);
           exit(0);
         }
      }
    }
  }

  else if(n_commands>1)
  {
     int server_socket, client_socket;
     int len, rc;
     struct sockaddr_un server_sockaddr;
     struct sockaddr_un client_sockaddr;
     char buf[256];
     memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
     memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));
     memset(buf, 0, 256);
     

     server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
 
     if(server_socket == -1)
     {
       printf("SERVER PROGRAM SERVER SOCKET MAKINT FAIL\n");
       exit(0);
     }
     else
     {
       printf("SERVER PROGRAM SERVER SOCKET MAKING SUCCESS\n");
     }
     
     server_sockaddr.sun_family = AF_UNIX;
     strcpy(server_sockaddr.sun_path, SOCK_PATH);
     len = sizeof(server_sockaddr);
     
     unlink(SOCK_PATH);
     rc = bind(server_socket, (struct sockaddr*)&server_sockaddr, len);
     if(rc==-1)
     {
       printf("SERVER PROGRAM BIND ERROR\n");
       close(server_socket);
       exit(0);
     }
     else
     {
       printf("SERVER PROGRAM BIND SUCCESS\n");
     }
    

  }

  return 0;
}

void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }

    free(argv);
  }

  memset((*commands), 0, sizeof(struct single_command) * n_commands);
}
