#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>


#define SOCK_PATH "tpf_unix_sock.server"


#include "commands.h"
#include "built_in.h"

#define SERVER_PATH "tpf_unix_sock.server"
#define CLIENT_PATH "tpf_unix_sock.client"

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

//client program
void* clientprogram (void *commandpart)
{
   struct single_command matrix[512];
   struct single_command* com1 = (struct single_command*)commandpart;
   matrix[0] = *com1;
   int client_socket;
   int rc, len;
   struct sockaddr_un server_sockaddr;
   struct sockaddr_un client_sockaddr;
   char buf[256];
   memset(&server_sockaddr, 0 , sizeof(struct sockaddr_un));
   memset(&client_sockaddr, 0 , sizeof(struct sockaddr_un));
   
   //client program's client socket part
   client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
   if(client_socket == -1)
   {
     printf("CLIENT PROGRAM CLIENT SOCKET MAKING FAIL\n");
     exit(0);
   }
   else
   {
     printf("CLIENT PROGRAM CLIENT SOCKET MAKING SUCCESS\n");
   }
    
   client_sockaddr.sun_family = AF_UNIX;
   strcpy(client_sockaddr.sun_path, CLIENT_PATH);
   len = sizeof(client_sockaddr);
   
   unlink(CLIENT_PATH);
   
   rc = bind(client_socket, (struct sockaddr*)&client_sockaddr, len);
   if(rc == -1)
   {
     printf("CLIENT PROGRAM BIND ERROR\n");
     close(client_socket);
     exit(0);
   }
   else
   {
     printf("CLIENT PROGRAM BIND SUCCESS\n");
   }


   //client program's server socket part
   server_sockaddr.sun_family = AF_UNIX;
   strcpy(server_sockaddr.sun_path, SERVER_PATH);
   rc = connect(client_socket, (struct sockaddr*)&server_sockaddr, len);
   if(rc == -1)
   {
     printf("CLIENT PROGRAM CONNECT ERROR\n");
     close(client_socket);
     exit(0);
   }
   /*else
   {
     printf("CLIENT PROGRAM CONNECT SUCCESS\n");
   }
   
   //sending mode..
     strcpy(buf, "client thread ok\n");
     printf("SENDING DATA TO SERVER PROGRAM FROM CLIENT PROGRAM\n");
     rc = send(client_socket, buf, strlen(buf), 0);
     if(rc == -1)
     {
       printf("SENDING ERROR\n");
       close(client_socket);
       exit(0);
     }
     else
     {
       printf("SENDING COMPLETE\n");
     }
   */
     close(STDIN_FILENO);
     //real start
     int stdo = dup(STDOUT_FILENO);
     rc = dup2(client_socket,STDOUT_FILENO);
     if(rc==-1)
     {
        printf("DUP2 FAILED\n");
        exit(0);
     }
     
     evaluate_command(1, &matrix);
     close(STDOUT_FILENO);
     dup2(stdo,STDOUT_FILENO);
      

     printf("test, this command : %s\n", *com1->argv);
     close(client_socket);
     pthread_exit(NULL);

}


/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  if(n_commands == 1)
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
      else if(pid>0)
      {
        wait(&pid);
      }
    }
  }

  else if(n_commands>1)
  {
     //MAKING SERVER PROGRAM
     
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

     //listen part
     int backlog = 5;
     void* status;
     rc = listen(server_socket, backlog);
     if(rc == -1)
     {
        printf("SERVER PROGRAM LISTEN ERROR\n");
        close(server_socket);
        exit(0);
     }
     printf("LISTENING..\n");
 

     //CLIENT THREAD MAKING
     pthread_t clientpart;
     int clientcheck;
     long t;
     struct single_command matrix[512];
      
     struct single_command *com1 = (*commands);
     struct single_command *com2 = &(*commands)[1];
     matrix[0] = *com2;
     clientcheck= pthread_create(&clientpart, NULL, clientprogram, (void*)(com1));
     if(clientcheck)
     {
       printf("THREAD CREATION FAIL\n");
       exit(0);
     }
     else
     {
       printf("THREAD CREATION SUCCESS\n");
     }
     
    sleep(1);

     //accept
     
     client_socket = accept(server_socket, (struct sockaddr*)&client_sockaddr, &len);
     if(client_socket == -1)
     {
        printf("ACCEPT ERROR\n");
        close(server_socket);
        close(client_socket);
        exit(0);
     }
     
     len = sizeof(client_sockaddr);
     rc = getpeername(client_socket, (struct sockaddr*)&client_sockaddr, &len);
     if(rc == -1)
     {
        printf("GETPEERNAME ERROR\n");
        close(server_socket);
        close(client_socket);
        exit(0);
     }
     else
     {
        printf("GETPEERNAME SUCCESS\n");
        printf("CLIENT SOCKET FILEPATH : %s\n", client_sockaddr.sun_path);
     }
 
     printf("waiting to read...\n");
     int bytes_rec = 0;
     /*bytes_rec = recv(client_socket, buf, sizeof(buf), 0);
     if(bytes_rec == -1)
     {
        printf("RECEVE ERROR\n");
        close(server_socket);
        close(client_socket);
        exit(0);
     }
     else
     {
        printf("DATA : %s\n", buf);
     }
     */
     //IPC final
     int fd;
     
     int state;
     close(STDOUT_FILENO);
     fd = dup(STDIN_FILENO);
     dup2(client_socket,STDIN_FILENO);
     pthread_join(clientpart, &state);
 
     evaluate_command(1,&matrix);
     close(STDIN_FILENO);
     dup2(fd,STDIN_FILENO);
     close(server_socket);
     close(client_socket);


     return 1;
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
