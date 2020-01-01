#include "peer.h"
#include "network.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// TODO: RIMUOVI! >>>>>>>>>>>>>>>>>>
#include "io.h"
#include "util.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

pthread_mutex_t peer_list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t count_upload_mutex = PTHREAD_MUTEX_INITIALIZER;

struct peer_list * active_peer_list = NULL;
struct download_list * active_download_list = NULL;

int got_stop = 0;
int count_upload = 0;

struct download_info
{
  char * filename;
  off_t file_size;
  off_t currently_downloaded;
  in_port_t peer_port;
  in_addr_t peer_addr;
};

struct download_list
{
  struct download_info * download;
  struct download_list * prev_download;
  struct download_list * next_download;
};

struct upload_thread_arg
{
  unsigned peer_fd;
  char filename[PATH_MAX];
};

struct download_thread_arg
{
  struct peer_list * active_peer_list;
  struct download_list ** active_download_list;
  char filename[PATH_MAX];
};

struct receive_peer_thread_arg
{
  struct peer_list ** active_peer_list;
  unsigned serv_fd;
  in_port_t peer_listen_port;
};

void sig_handler(int signal)
{
  if(signal == SIGUSR1)
  {
    got_stop = 2;

    pthread_mutex_lock(&download_list_mutex);
    active_download_list = free_download_list(active_download_list);
    pthread_mutex_unlock(&download_list_mutex);

    pthread_mutex_lock(&peer_list_mutex);
    active_peer_list = free_peer_list(active_peer_list);
    pthread_mutex_unlock(&peer_list_mutex);

    raise(SIGINT);
  }

  return;
}

void * upload_thread(void * arg)
{
  struct upload_thread_arg u_info = {
    .peer_fd = ((struct upload_thread_arg *) arg) -> peer_fd,
  };

  strcpy(u_info.filename, ((struct upload_thread_arg *) arg) -> filename);

  try_upload_file_to_one_peer(u_info.peer_fd, u_info.filename, &count_upload);

  close(u_info.peer_fd);

  if(got_stop == 1)
  {
    raise(SIGUSR1);
  }

  pthread_exit(NULL);
}

void * handle_upload_thread(void * arg)
{
  const int peer_fd = *(int *) arg;

  while(1)
  {
    char filename[PATH_MAX] = {'\0'};
    size_t filename_len = 0;

    read(peer_fd, &filename_len, sizeof filename_len);
    read(peer_fd, filename, filename_len);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd > 0)
    {
      struct sockaddr_in sin = {
        .sin_family = AF_INET,
        .sin_port = 0,
        .sin_addr = {
          .s_addr = htonl(INADDR_ANY)
        }
      };

      if(!bind(sockfd, (struct sockaddr *)&sin, sizeof sin))
      {
        socklen_t len = sizeof sin;

        if(!getsockname(sockfd, (struct sockaddr *)&sin, &len))
        {
          write(peer_fd, &sin.sin_port, sizeof sin.sin_port);
          //printf("ho appena scritto la porta : %d\n", sin.sin_port);

          if(!listen(sockfd, SOMAXCONN))
          {
            struct sockaddr_in client_sin;
            int clientfd = 0;
            len = sizeof client_sin;

            if((clientfd = accept(sockfd, (struct sockaddr *)&client_sin, &len)) > 0)
            {
              struct upload_thread_arg u_info = {
                .peer_fd = clientfd,
              };

              strcpy(u_info.filename, filename);

              pthread_t u_tid;
              pthread_attr_t u_attr;

              if(!pthread_attr_init(&u_attr))
              {
                if(!pthread_attr_setdetachstate(&u_attr, PTHREAD_CREATE_DETACHED))
                {
                  if(pthread_create(&u_tid, &u_attr, upload_thread, &u_info))
                  {
                    //printf("problemi\n");
                  }
                }
              }
            }
            else
            {
              perror("accept()");
            }
          }
          else
          {
            perror("listen()");
          }

        }
        else
        {
          int flag = -1;

          write(peer_fd, &flag, sizeof flag);
        }


      }
      else
      {
        perror("bind()");
      }

      close(sockfd);
    }
    else
    {
      perror("socket()");
    }
  }

  pthread_exit(NULL);
}

void * download_thread(void * arg)
{
  struct download_thread_arg d_info = {
    .active_peer_list = ((struct download_thread_arg *)arg) -> active_peer_list,
    .active_download_list = ((struct download_thread_arg *)arg) -> active_download_list
  };

  int download_count = 0;
  strcpy(d_info.filename, ((struct download_thread_arg *)arg) -> filename);

  struct peer_list * download_peer_info = NULL;

  if(d_info.active_peer_list)
  {
    if(d_info.filename)
    {
      download_peer_info = try_download_file_from_any_peer(d_info.active_peer_list, d_info.active_download_list, d_info.filename);
    }
  }

  //printf("********************\n*******USCENDO DAL DOWNLOADN THREAD*****************\n");

  if(download_peer_info)
  {
    pthread_mutex_lock(&download_list_mutex);
    *d_info.active_download_list = discard_download_node(*d_info.active_download_list, d_info.filename, download_peer_info -> peer -> sin.sin_addr.s_addr, download_peer_info -> peer -> sin.sin_port);
    pthread_mutex_unlock(&download_list_mutex);
  }

  if(got_stop == 1)
  {
    pthread_mutex_lock(&count_upload_mutex);

    if((download_count = count_download(*d_info.active_download_list)) == 0 && count_upload == 0)
    {
      raise(SIGUSR1);
    }

    pthread_mutex_unlock(&count_upload_mutex);
  }

  pthread_exit(NULL);
}

void * accept_thread(void * arg)
{
  in_port_t peer_listen_port = *(in_port_t *)arg;
  int peer_listen_fd = -1;

  if((peer_listen_fd = socket(AF_INET, SOCK_STREAM, 0)) > 0)
  {
    struct sockaddr_in peer_listen_sin = {
      .sin_family = AF_INET,
      .sin_port = peer_listen_port,
      .sin_addr = {
        .s_addr = htonl(INADDR_ANY)
      }
    };

    if(!bind(peer_listen_fd, (const struct sockaddr *) &peer_listen_sin, sizeof peer_listen_sin))
    {
      if(!listen(peer_listen_fd, SOMAXCONN))
      {
        int peer_fd = -1;
        struct sockaddr_in peer_info;
        socklen_t peer_info_size = sizeof peer_info;

        while(1)
        {
          //printf("IN ATTESA DI ACCETTARE NUOVI PEER SULL'FD %d\n", peer_listen_fd);

          if((peer_fd = accept(peer_listen_fd, (struct sockaddr *) &peer_info, &peer_info_size)) == -1)
          {
            perror("accept()");
          }
          else
          {
            pthread_t h_upload_tid;
            pthread_attr_t h_upload_attr;

            if(!pthread_attr_init(&h_upload_attr))
            {
              if(!pthread_attr_setdetachstate(&h_upload_attr, PTHREAD_CREATE_DETACHED))
              {
                if(pthread_create(&h_upload_tid, &h_upload_attr, handle_upload_thread, &peer_fd))
                {
                  //printf("Impossibile creare il thread di Upload dei file\n");
                }
              }
            }
          }
        }
      }
      else
      {
        perror("listen()");
      }
    }
    else
    {
      perror("bind()");
    }
  }

  pthread_exit(NULL);
}

void * receive_peer_thread(void * arg)
{
  // pthread_mutex_lock(&peer_list_mutex);

  struct receive_peer_thread_arg info = {
    .peer_listen_port = ((struct receive_peer_thread_arg *)arg) -> peer_listen_port,
    .serv_fd = ((struct receive_peer_thread_arg *)arg) -> serv_fd,
    .active_peer_list = ((struct receive_peer_thread_arg *)arg) -> active_peer_list
  };

  // pthread_mutex_unlock(&peer_list_mutex);

  // 1) Mando la porta su cui mi metto in attesa di connessioni al server di rendez-vous
  write(info.serv_fd, &info.peer_listen_port, sizeof info.peer_listen_port);


  // 2) Aspetto la lista dei peer precedentemente connessi
  receive_active_peer_list(info.active_peer_list, info.serv_fd);


  while(1)
  {
    receive_peer_info(info.active_peer_list, info.serv_fd);
  }

  pthread_exit(NULL);
}

struct download_info * create_download_info(const char * filename, const off_t file_size, const in_addr_t peer_addr, in_port_t peer_port)
{
  struct download_info * new_download_info = (struct download_info *)calloc(1, sizeof * new_download_info);

  if(new_download_info)
  {
    if(filename) //TODO: puo' essere superfluo?
    {
      new_download_info -> filename = malloc((strlen(filename) + 1) * sizeof(char));
      strcpy(new_download_info -> filename, filename);
    }

    new_download_info -> file_size = file_size;
    new_download_info -> currently_downloaded = 0;
    new_download_info -> peer_port = peer_port;
    new_download_info -> peer_addr = peer_addr;
  }

  return new_download_info;
}

struct download_info * delete_download_info(struct download_info * download_info)
{
  if(download_info)
  {
    free(download_info -> filename);
    memset(&download_info, 0, sizeof download_info);
    free(download_info);
    download_info = NULL;
  }

  return download_info;
}

struct download_list * search_download_node(struct download_list * head, const char * filename, const in_addr_t peer_addr, const in_port_t peer_port)
{
  struct download_list * ret_node;

  if(head)
  {
    if(!strcmp(head -> download -> filename, filename) && head -> download -> peer_addr == peer_addr && head -> download -> peer_port == peer_port)
    {
      ret_node = head;
    }
    else
    {
      ret_node = search_download_node(head -> next_download, filename, peer_addr, peer_port);
    }
  }
  else
  {
    ret_node = NULL;
  }

  return ret_node;
}

struct download_list * add_download_info(struct download_list * head, const char * filename, const off_t file_size, const in_addr_t peer_addr, const in_port_t peer_port)
{
  struct download_list * new_node = (struct download_list *)calloc(1, sizeof new_node);

  if(new_node)
  {
    struct download_info * new_info = create_download_info(filename, file_size, peer_addr, peer_port);

    if(new_info)
    {
      new_node -> download = new_info;
      new_node -> next_download = head;
      new_node -> prev_download = NULL;

      if(head != NULL)
      {
        head -> prev_download = new_node;
      }
    }
  }

  return new_node;
}

void update_download_info(struct download_list * head, const char * filename, const in_addr_t peer_addr, const in_port_t peer_port, const off_t currently_downloaded)
{
  struct download_list * download_node = search_download_node(head, filename, peer_addr, peer_port);

  if(download_node)
  {
    if(download_node -> download)
    {
      download_node -> download -> currently_downloaded = currently_downloaded;
    }
  }

  return;
}

int count_download(struct download_list * head)
{
  int count = 0;

  for(count = 0; head != NULL; head = head -> next_download, count++);

  return count;
}

struct download_list * discard_download_info(struct download_list * node)
{
  if(node)
  {
    node -> prev_download = NULL;
    node -> download = delete_download_info(node -> download);
    node -> next_download = NULL;
    memset(&node, 0, sizeof node);
    free(node);
    node = NULL;
  }

  return node;
}

struct download_list * discard_download_node(struct download_list * head, const char * filename, const in_addr_t peer_addr, const in_port_t peer_port)
{
  struct download_list * node_to_delete = search_download_node(head, filename, peer_addr, peer_port);

  //printf("trovato il nodo da rimuovere, ha porta %hu\n", ntohs(node_to_delete -> download -> peer_port));

  if(node_to_delete)
  {
    if(node_to_delete -> prev_download)
    {
      if(node_to_delete -> next_download)
      {
        node_to_delete -> prev_download -> next_download = node_to_delete -> next_download;
        node_to_delete -> next_download -> prev_download = node_to_delete -> prev_download;
        node_to_delete = discard_download_info(node_to_delete);
      }
      else
      {
        node_to_delete -> prev_download -> next_download = NULL;
        node_to_delete = discard_download_info(node_to_delete);
      }
    }
    else
    {
      if(node_to_delete -> next_download)
      {
        node_to_delete -> next_download -> prev_download = NULL;
      }

      head = node_to_delete -> next_download;
      node_to_delete = discard_download_info(node_to_delete);
    }
  }

  return head;
}

struct download_list * free_download_list(struct download_list * head)
{
  if(head)
  {
    free_download_list(head -> next_download);
    head = discard_download_node(head, head -> download -> filename, head -> download -> peer_addr, head -> download -> peer_port);
  }

  return head;
}

void show_download_list(struct download_list * head)
{
  char tmp[4096] = {'\0'};

  sprintf(tmp,"\n\n\t   **********%d DOWNLOAD IN CORSO**********\n\n", count_download(head));

  write(STDOUT_FILENO, tmp, strlen(tmp));

  if(head != NULL)
  {
    write(STDOUT_FILENO, "NOME DEL FILE\t\t\tDIMENSIONE DEL FILE\t\t\tPERCENTUALE DI COMPLETAMENTO\t\t\tPEER\n\n\n", strlen("NOME DEL FILE\t\t\tDIMENSIONE DEL FILE\t\t\tPERCENTUALE DI COMPLETAMENTO\t\t\tPEER\n\n\n"));
    char buffer[4096] = {0};

    while(head != NULL)
    {
      char ip[INET_ADDRSTRLEN] = {0};

      if(inet_ntop(AF_INET, &(head -> download -> peer_addr), ip, 4096) != NULL)
      {
        sprintf(buffer, "%s%38ld%43.1f%%%38s:%hu\t\t\t\n\n", head -> download -> filename,
        head -> download -> file_size,
        ((float)head -> download -> currently_downloaded / (float)head -> download -> file_size) * 100.0f,
        ip,
        ntohs(head -> download -> peer_port));

        write(STDOUT_FILENO, buffer, strlen(buffer));
        memset(buffer, 0, 4096);
      }
      else
      {
        perror("inet_ntop()");
        exit(-1);
      }

      head = head -> next_download;
    }
  }

  write(STDOUT_FILENO, "\n\n\t\t    ***********\n\n", strlen("\n\n\t\t    ***********\n\n"));

  return;
}

void receive_active_peer_list(struct peer_list ** head, const unsigned serv_fd)
{
  int active_peer = 0;

  read(serv_fd, &active_peer, sizeof active_peer);

  for(int i = 0; i < active_peer; ++i)
  {
    in_addr_t peer_addr;
    in_port_t peer_port;
    enum peer_status peer_status;

    read(serv_fd, &peer_addr, sizeof peer_addr);
    read(serv_fd, &peer_port, sizeof peer_port);
    read(serv_fd, &peer_status, sizeof peer_status);

    // pthread_mutex_lock(&peer_list_mutex);
    *head = add_peer_node(*head, peer_addr, peer_port, peer_status, 0);
    // pthread_mutex_unlock(&peer_list_mutex);

    if(peer_status != TERMINAZIONE)
    {
      // pthread_mutex_lock(&peer_list_mutex);
      get_connected_to_last_peer(head);
      // pthread_mutex_unlock(&peer_list_mutex);
    }
  }

  return;
}

void receive_peer_info(struct peer_list ** head, const unsigned serv_fd)
{
  in_addr_t peer_addr;
  in_port_t peer_port;

  memset(&peer_addr, 0, sizeof peer_addr);
  memset(&peer_port, 0, sizeof peer_port);

  read(serv_fd, &peer_addr, sizeof peer_addr);
  read(serv_fd, &peer_port, sizeof peer_port);

  struct peer_list * retrieve_node = NULL;

  pthread_mutex_lock(&peer_list_mutex);

  if((retrieve_node = search_peer_node(*head, peer_addr, peer_port)))
  {
    enum peer_status peer_status;

    if((peer_status = get_peer_status(retrieve_node)) == TERMINAZIONE)
    {
      *head = discard_peer_node(*head, retrieve_node -> peer -> sin.sin_addr.s_addr, retrieve_node -> peer -> sin.sin_port);
    }
    else
    {
      change_peer_status(*head, retrieve_node -> peer -> sin.sin_addr.s_addr, retrieve_node -> peer -> sin.sin_port, TERMINAZIONE);
    }
  }
  else
  {
    *head = add_peer_node(*head, peer_addr, peer_port, ATTIVO, 0);
    get_connected_to_last_peer(head);
  }

  pthread_mutex_unlock(&peer_list_mutex);

  return;
}

int get_peer_status(const struct peer_list * peer_node)
{
  return peer_node -> peer -> peer_status;
}

void get_connected_to_last_peer(struct peer_list ** head)
{
  if(((*head) -> peer -> peer_fd = socket(AF_INET, SOCK_STREAM, 0)) > 0)
  {
    struct sockaddr_in peer = {
      .sin_family = AF_INET,
      .sin_port = (*head) -> peer -> sin.sin_port,
      .sin_addr = {
        .s_addr = (*head) -> peer -> sin.sin_addr.s_addr
      }
    };

    //printf("sto per entrare in connect\n");

    if(!connect((*head) -> peer -> peer_fd, (struct sockaddr *)&peer, sizeof peer))
    {
      //printf("mi sono appena connesso al peer %u:%hu\n", peer.sin_addr.s_addr, ntohs(peer.sin_port));
    }
    else
    {
      perror("connect()");
    }
  }
  else
  {
    perror("socket()");
  }

  return;
}

int init_peer(const struct sockaddr_in * server, const in_port_t peer_listen_port)
{
  int serv_fd = -1;

  if((serv_fd = socket(AF_INET, SOCK_STREAM, 0)) > 0)
  {
    if(!connect(serv_fd, (const struct sockaddr *) &(*server), sizeof * server))
    {
      pthread_t accept_tid;
      pthread_attr_t accept_attr;

      if(!pthread_attr_init(&accept_attr))
      {
        if(!pthread_attr_setdetachstate(&accept_attr, PTHREAD_CREATE_DETACHED))
        {
          if(pthread_create(&accept_tid, &accept_attr, accept_thread, &peer_listen_port))
          {
            //printf("Impossibile creare accept_thread\n");
          }
          else
          {
            pthread_t receive_peer_tid;
            pthread_attr_t receive_peer_attr;

            // pthread_mutex_lock(&peer_list_mutex);

            struct receive_peer_thread_arg info = {
              .active_peer_list = &active_peer_list,
              .serv_fd = serv_fd,
              .peer_listen_port = peer_listen_port
            };

            // pthread_mutex_unlock(&peer_list_mutex);

            if(!pthread_attr_init(&receive_peer_attr))
            {
              if(!pthread_attr_setdetachstate(&receive_peer_attr, PTHREAD_CREATE_DETACHED))
              {
                if(pthread_create(&receive_peer_tid, &receive_peer_attr, receive_peer_thread, &info))
                {
                  //printf("Impossibile creare receive_peer_thread\n");
                }
                else
                {
                  char str[PATH_MAX] ={'\0'};

                  int nread = 0;

                  signal(SIGUSR1, sig_handler);

                  while((nread = read(STDIN_FILENO, str, PATH_MAX)) > 0)
                  {
                    if(strcmp("connessioni\n", str) == 0)
                    {
                      pthread_mutex_lock(&peer_list_mutex);
                      show_peer_list(active_peer_list);
                      pthread_mutex_unlock(&peer_list_mutex);
                    }
                    else if(strcmp("download\n", str) == 0)
                    {
                      pthread_mutex_lock(&download_list_mutex);
                      show_download_list(active_download_list);
                      pthread_mutex_unlock(&download_list_mutex);
                    }
                    else if(strcmp("stop\n", str) == 0)
                    {
                      write(serv_fd, str, strlen(str));
                      got_stop = 1;

                      pthread_mutex_lock(&count_upload_mutex);

                      if(count_download(active_download_list) == 0 && count_upload == 0)
                      {
                        raise(SIGUSR1);
                      }

                      pthread_mutex_unlock(&count_upload_mutex);
                    }
                    else
                    {
                      //printf("ricevuto %s\n", str);

                      if(got_stop == 0)
                      {
                        pthread_t download_tid;
                        pthread_attr_t download_attr;

                        pthread_mutex_lock(&peer_list_mutex);
                        pthread_mutex_lock(&download_list_mutex);

                        struct download_thread_arg d_info = {
                          .active_peer_list = active_peer_list,
                          .active_download_list = &active_download_list,
                        };

                        pthread_mutex_unlock(&download_list_mutex);
                        pthread_mutex_unlock(&peer_list_mutex);

                        str[strcspn(str, "\n")] = '\0';

                        strcpy(d_info.filename, str);

                        if(!pthread_attr_init(&download_attr))
                        {
                          if(!pthread_attr_setdetachstate(&download_attr, PTHREAD_CREATE_DETACHED))
                          {
                            if(pthread_create(&download_tid, &download_attr, download_thread, &d_info))
                            {
                              //printf("Problemi nella creazione del thread di download.");
                            }
                          }
                        }
                      }
                    }

                    memset(str, 0, PATH_MAX);
                  }

                  close(serv_fd);
                }
              }
            }
          }
        }
      }
    }
    else
    {
      perror("connect()");
    }
  }
  else
  {
    perror("socket()");
  }

  return serv_fd;
}
