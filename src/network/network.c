#include "network.h"
#include "io.h"
#include "util.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include "peer.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef BUFSIZE
#define BUFSIZE 100
#endif

pthread_mutex_t download_list_mutex = PTHREAD_MUTEX_INITIALIZER;

struct peer * create_peer_info(const in_addr_t peer_addr, const in_port_t peer_port, enum peer_status peer_status, const int peer_fd)
{
  struct peer * new_peer = (struct peer *)calloc(1, sizeof * new_peer);

  if(new_peer)
  {
    new_peer -> sin.sin_port = peer_port;
    new_peer -> sin.sin_addr.s_addr = peer_addr;
    new_peer -> peer_fd = peer_fd;
    new_peer -> peer_status = peer_status;
  }

  return new_peer;
}

struct peer * delete_peer_info(struct peer * peer)
{
  if(peer)
  {
    peer -> peer_fd = -1;
    memset(&peer, 0, sizeof peer);
    free(peer);
    peer = NULL;
  }

  return peer;
}

struct peer_list * search_peer_node(struct peer_list * head, const in_addr_t ip, const in_port_t port)
{
  struct peer_list * ret_node;

  if(head)
  {
    if(head -> peer -> sin.sin_addr.s_addr == ip && head -> peer -> sin.sin_port == port)
    {
      ret_node = head;
      // //printf("Trovato\n");
    }
    else
    {
      ret_node = search_peer_node(head -> next_peer, ip, port);
      // //printf("Lo sto ancora cercando!\n");
    }
  }
  else
  {
    ret_node = NULL;
    // //printf("Non trovato\n");
  }

  return ret_node;
}

struct peer_list * add_peer_node(struct peer_list * head, const in_addr_t peer_addr, const in_port_t peer_port, enum peer_status peer_status, const int peer_fd)
{
  struct peer_list * new_node = (struct peer_list *)calloc(1, sizeof new_node);

  if(new_node)
  {
      struct peer * new_peer = create_peer_info(peer_addr, peer_port, peer_status, peer_fd);

      if(new_peer)
      {
        new_node -> peer = new_peer;
        new_node -> next_peer = head;
        new_node -> prev_peer = NULL;

        if(head != NULL)
        {
          head -> prev_peer = new_node;
        }
      }
  }

  return new_node;
}

int count_peer_node(struct peer_list * head)
{
  int count = 0;

  for(count = 0; head != NULL; head = head -> next_peer, count++);

  return count;
}

struct peer_list * discard_peer_node(struct peer_list * head, const in_addr_t peer_addr, const in_port_t peer_port)
{
  struct peer_list * node_to_delete = search_peer_node(head, peer_addr, peer_port);

  //printf("trovato il nodo da rimuovere, ha porta %hu\n", ntohs(node_to_delete -> peer -> sin.sin_port));

  if(node_to_delete)
  {
    if(node_to_delete -> prev_peer)
    {
      if(node_to_delete -> next_peer)
      {
        node_to_delete -> prev_peer -> next_peer = node_to_delete -> next_peer;
        node_to_delete -> next_peer -> prev_peer = node_to_delete -> prev_peer;
        node_to_delete = discard_peer_info(node_to_delete);
      }
      else
      {
        node_to_delete -> prev_peer -> next_peer = NULL;
        node_to_delete = discard_peer_info(node_to_delete);
      }
    }
    else
    {
      if(node_to_delete -> next_peer)
      {
        node_to_delete -> next_peer -> prev_peer = NULL;
      }

      head = node_to_delete -> next_peer;
      node_to_delete = discard_peer_info(node_to_delete);
    }
  }

  return head;
}

struct peer_list * discard_peer_info(struct peer_list * node)
{
  if(node)
  {
    node -> prev_peer = NULL;
    node -> peer = delete_peer_info(node -> peer);
    node -> next_peer = NULL;
    memset(&node, 0, sizeof node);
    free(node);
    node = NULL;
  }

  return node;
}

struct peer_list * free_peer_list(struct peer_list * head)
{
  if(head)
  {
    free_peer_list(head -> next_peer);
    // TODO : se abbiamo tempo, implementare a parte
    head = discard_peer_node(head, head -> peer -> sin.sin_addr.s_addr, head -> peer -> sin.sin_port);
  }

  return head;
}

void show_peer_list(struct peer_list * head)
{
  write(STDOUT_FILENO, "\n\n\t    *****LISTA PEER ATTIVI*****\n\n", strlen("\n\n\t    *****LISTA PEER ATTIVI*****\n\n"));

  if(head != NULL)
  {
    write(STDOUT_FILENO, "IP\t\t\tPORTA\t\t\tSTATO DEL PEER\n\n\n", strlen("IP\t\t\tPORTA\t\t\tSTATO DEL PEER\n\n\n"));
    char buffer[BUFSIZE] = {0};

    while(head != NULL)
    {
      if(inet_ntop(AF_INET, &(head -> peer -> sin.sin_addr.s_addr), buffer, 100) != NULL)
      {
        sprintf(buffer, "%s\t\t%hu\t\t\t", buffer, ntohs(head -> peer -> sin.sin_port));

        if(head -> peer -> peer_status == ATTIVO)
        {
          strcat(buffer, "attivo\n");
        }
        else if(head -> peer -> peer_status == TERMINAZIONE)
        {
          strcat(buffer, "in terminazione\n");
        }
        else
        {
          strcat(buffer, "in download\n");
        }

        write(STDOUT_FILENO, buffer, strlen(buffer));
        memset(buffer, 0, 100);
      }
      else
      {
        perror("inet_ntop()");
        exit(-1);
      }

      head = head -> next_peer;
    }
  }

  write(STDOUT_FILENO, "\n\n\t\t    ***********\n\n", strlen("\n\n\t\t    ***********\n\n"));
}

void change_peer_status(struct peer_list * head, const in_addr_t peer_addr, in_port_t peer_port, enum peer_status new_status)
{
  struct peer_list * peer_node = search_peer_node(head, peer_addr, peer_port);

  //printf("sono change peer status e sono uscito dalla ricerca\n");

  if(peer_node)
  {
    if(peer_node -> peer)
    {
      peer_node -> peer -> peer_status = new_status;
    }
  }

  return;
}

off_t try_download_file_from_one_peer(struct download_list ** active_download_list, const unsigned upload_peer_fd, const char * filename, const in_addr_t peer_addr, const in_port_t peer_port)
{
  size_t filename_len = strlen(filename);
  off_t file_size = 0;
  int flag = 0;

  write(upload_peer_fd, &filename_len, sizeof filename_len);
  write(upload_peer_fd, filename, filename_len);

  read(upload_peer_fd, &flag, sizeof flag);
  //printf("ho appena ricevuto questa porta : %d\n", flag);

  if(flag != -1)
  {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd > 0)
    {
      struct sockaddr_in sin_server = {
        .sin_family = AF_INET,
        .sin_port = flag,
        .sin_addr = {
          .s_addr = peer_addr
        }
      };

      if(!connect(sockfd, (struct sockaddr *)&sin_server, sizeof sin_server))
      {
        read(sockfd, &file_size, sizeof file_size);

        if(file_size > 0)
        {
          pthread_mutex_lock(&download_list_mutex);
          *active_download_list = add_download_info(*active_download_list, filename, file_size, peer_addr, peer_port);
          // //printf("sono riuscito ad aggiungere il nodo\n");
          pthread_mutex_unlock(&download_list_mutex);

          char * file_path = calloc(1, (filename_len + strlen(download_dir_path) + 1) * sizeof(char));

          sprintf(file_path, "%s/%s", download_dir_path, filename);

          read_file(active_download_list, sockfd, file_size, file_path, filename, peer_addr, peer_port);
        }

        close(sockfd);
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


  }

  return file_size;
}

struct peer_list * try_download_file_from_any_peer(struct peer_list * active_peer_list, struct download_list ** active_download_list, const char * filename)
{
  off_t file_size = 0;
  struct peer_list * ret = NULL;

  while(active_peer_list && !file_size)
  {
    if(active_peer_list -> peer -> peer_status != TERMINAZIONE)
    {
      if(!(file_size = try_download_file_from_one_peer(active_download_list, active_peer_list -> peer -> peer_fd, filename, active_peer_list -> peer -> sin.sin_addr.s_addr, active_peer_list -> peer -> sin.sin_port)))
      {
        active_peer_list = active_peer_list -> next_peer;
      }
      else
      {
        ret = active_peer_list;
      }
    }
    else
    {
      active_peer_list = active_peer_list -> next_peer;
    }
  }

  return ret;
}

void try_upload_file_to_one_peer(const unsigned download_peer_fd, const char * filename, int * count_upload)
{
  char * filepath = NULL;
  off_t file_size = 0;

  if((file_size = file_exist(filename, &filepath)))
  {
    write(download_peer_fd, &file_size, sizeof file_size);

    //printf("sono il thread di upload, ho trovato il file %s ed ora lo invio\n", filepath);

    write_file(download_peer_fd, filepath, count_upload);

    memset(filepath, 0, sizeof * filepath);
  }
  else
  {
    write(download_peer_fd, &file_size, sizeof file_size);
  }

  // memset(filename, 0, PATH_MAX);


  return;
}
