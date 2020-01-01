#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

struct peer_list;
struct client_thread_arg;

void * client_thread(void *);

void send_active_peer_list(struct peer_list *, const in_addr_t, const in_port_t, const unsigned);

void send_last_connected_client(struct peer_list *, const in_addr_t, const in_port_t);

void send_changed_status_peer(struct peer_list *, const in_addr_t, const in_port_t);

void send_peer_to_discard(struct peer_list *, const in_addr_t, const in_port_t);

int init_server(const struct sockaddr_in *);

#endif
