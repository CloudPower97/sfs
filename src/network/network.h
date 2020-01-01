#ifndef NETWORK_H
#define NETWORK_H

#include <arpa/inet.h>
#include <pthread.h>

extern pthread_mutex_t download_list_mutex;

enum peer_status
{
  ATTIVO,
  DOWNLOAD,
  TERMINAZIONE,
};

struct peer
{
  struct sockaddr_in sin;
  int peer_fd;
  enum peer_status peer_status;
};

struct peer_list
{
  struct peer * peer;
  struct peer_list * prev_peer;
  struct peer_list * next_peer;
};

struct download_list;

struct peer * create_peer_info(const in_addr_t, const in_port_t, enum peer_status, const int);

struct peer * delete_peer_info(struct peer *);

struct peer_list * search_peer_node(struct peer_list *, const in_addr_t, const in_port_t);

struct peer_list * add_peer_node(struct peer_list *, const in_addr_t, const in_port_t, enum peer_status, const int);

int count_peer_node(struct peer_list *);

struct peer_list * discard_peer_node(struct peer_list *, const in_addr_t, const in_port_t);

struct peer_list * discard_peer_info(struct peer_list *);

struct peer_list * free_peer_list(struct peer_list *);

void show_peer_list(struct peer_list *);

void change_peer_status(struct peer_list *, const in_addr_t, const in_port_t, enum peer_status);

off_t try_download_file_from_one_peer(struct download_list **, const unsigned, const char *, const in_addr_t, const in_port_t);

struct peer_list * try_download_file_from_any_peer(struct peer_list *, struct download_list **, const char *);

void try_upload_file_to_one_peer(const unsigned, const char *, int *);

#endif
