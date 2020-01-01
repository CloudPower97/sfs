#include "server.h"
#include "network.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t client_list_mutex = PTHREAD_MUTEX_INITIALIZER;

struct client_thread_arg
{
  in_addr_t client_addr;
  unsigned client_fd;
  struct peer_list ** active_peer_list;
};

void * client_thread(void * arg)
{
  struct client_thread_arg * client_info = (struct client_thread_arg *) arg;
  unsigned client_fd = client_info -> client_fd;
  in_addr_t client_addr = client_info -> client_addr;
  in_port_t client_port;
  char stop[6] = {'\0'};

  read(client_fd, &client_port, sizeof client_port);

  pthread_mutex_lock(&client_list_mutex);

  *(client_info -> active_peer_list) = add_peer_node(*(client_info -> active_peer_list), client_addr, client_port, ATTIVO, client_fd);

  show_peer_list(*client_info -> active_peer_list);

  send_active_peer_list(*client_info -> active_peer_list, client_addr, client_port, client_fd);

  send_last_connected_client(*client_info -> active_peer_list, client_addr, client_port);

  pthread_mutex_unlock(&client_list_mutex);

  read(client_fd, stop, sizeof stop);

  pthread_mutex_lock(&client_list_mutex);

  change_peer_status(*client_info -> active_peer_list, client_addr, client_port, TERMINAZIONE);

  send_changed_status_peer(*client_info -> active_peer_list, client_addr, client_port);

  show_peer_list(*client_info -> active_peer_list);

  pthread_mutex_unlock(&client_list_mutex);

  //printf("Leggo dall'fd :%u\n", client_fd);

  read(client_fd, stop, sizeof stop);

  pthread_mutex_lock(&client_list_mutex);

  *client_info -> active_peer_list = discard_peer_node(*client_info -> active_peer_list,client_addr, client_port);

  send_peer_to_discard(*client_info -> active_peer_list, client_addr, client_port);

  show_peer_list(*client_info -> active_peer_list);

  pthread_mutex_unlock(&client_list_mutex);

  pthread_exit(NULL);
}

void send_last_connected_client(struct peer_list * head, const in_addr_t last_client_addr, const in_port_t last_client_port)
{
  while(head)
  {
    if(last_client_addr != head -> peer -> sin.sin_addr.s_addr || last_client_port != head -> peer -> sin.sin_port)
    {
      write(head -> peer -> peer_fd, &last_client_addr, sizeof last_client_addr);
      write(head -> peer -> peer_fd, &last_client_port, sizeof last_client_port);
    }

    head = head -> next_peer;
  }

  return;
}

void send_active_peer_list(struct peer_list * head, const in_addr_t client_addr, const in_port_t client_port, const unsigned client_fd)
{
  int active_peer = count_peer_node(head) - 1;

  write(client_fd, &active_peer, sizeof active_peer);

  while(head)
  {
    if(client_addr != head -> peer -> sin.sin_addr.s_addr || client_port != head -> peer -> sin.sin_port)
    {
      write(client_fd, &(head -> peer -> sin.sin_addr.s_addr), sizeof head -> peer -> sin.sin_addr.s_addr);
      write(client_fd, &(head -> peer -> sin.sin_port), sizeof head -> peer -> sin.sin_port);
      write(client_fd, &(head -> peer -> peer_status), sizeof head -> peer -> peer_status);
    }

    head = head -> next_peer;
  }

  return;
}

void send_changed_status_peer(struct peer_list * head, const in_addr_t changed_peer_addr, const in_port_t changed_peer_port)
{
  while(head)
  {
    if(changed_peer_addr != head -> peer -> sin.sin_addr.s_addr || changed_peer_port != head -> peer -> sin.sin_port)
    {
      write(head -> peer -> peer_fd, &changed_peer_addr, sizeof changed_peer_addr);
      write(head -> peer -> peer_fd, &changed_peer_port, sizeof changed_peer_port);
    }

    head = head -> next_peer;
  }

  return;
}

void send_peer_to_discard(struct peer_list * head, const in_addr_t discarding_peer_addr, const in_port_t discarding_peer_port)
{
  while(head)
  {
    if(discarding_peer_addr != head -> peer -> sin.sin_addr.s_addr || discarding_peer_port != head -> peer -> sin.sin_port)
    {
      write(head -> peer -> peer_fd, &discarding_peer_addr, sizeof discarding_peer_addr);
      write(head -> peer -> peer_fd, &discarding_peer_port, sizeof discarding_peer_port);
    }

    head = head -> next_peer;
  }

  return;
}

int init_server(const struct sockaddr_in * server)
{
  int sock_fd = -1;
  int exit_status = 0;

  if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) > 0)
  {
    if(!bind(sock_fd, (const struct sockaddr *) &(*server), sizeof * server))
    {
      if(!listen(sock_fd, SOMAXCONN))
      {
        struct peer_list * active_peer_list = NULL;
        int client_fd = -1;
        struct sockaddr_in client;
        socklen_t client_len = sizeof client;

        exit_status = 1;

        while(1)
        {
          if((client_fd = accept(sock_fd, (struct sockaddr *) &client, &client_len)) > 0)
          {
            pthread_t client_tid;
            pthread_attr_t  client_attr;

            if(!pthread_attr_init(&client_attr))
            {
              if(!pthread_attr_setdetachstate(&client_attr, PTHREAD_CREATE_DETACHED))
              {
                struct client_thread_arg client_info = {
                  .client_addr = client.sin_addr.s_addr,
                  .client_fd = client_fd,
                  .active_peer_list = &active_peer_list
                };

                if(pthread_create(&client_tid, &client_attr, client_thread, &client_info))
                {
                  //printf("Problemi nella creazione del thread relativo al peer appena connesso\n");
                }
              }
            }
          }
          else
          {
            perror("accept()");
          }
        }

        active_peer_list = free_peer_list(active_peer_list);
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
  else
  {
    perror("socket()");
  }

  return exit_status;
}
