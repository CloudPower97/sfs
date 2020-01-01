#ifndef PEER_H
#define PEER_H

#include <netinet/ip.h>

struct download_info;
struct download_list;
struct peer_list;
struct download_thread_arg;


struct download_info * create_download_info(const char *, const off_t, const in_addr_t, const in_port_t);

struct download_info * delete_download_info(struct download_info *);

struct download_list * search_download_node(struct download_list *, const char *, const in_addr_t, const in_port_t);

struct download_list * add_download_info(struct download_list *, const char *, const off_t, const in_addr_t, const in_port_t);

void update_download_info(struct download_list *, const char *, const in_addr_t, const in_port_t, const off_t);

int count_download(struct download_list *);

struct download_list * discard_download_info(struct download_list *);

struct download_list * discard_download_node(struct download_list *, const char *, const in_addr_t, const in_port_t);

struct download_list * free_download_list(struct download_list *);

void show_download_list(struct download_list *);

void sig_handler(int);

void * download_thread(void *);

void * upload_thread(void *);

void * handle_upload_thread(void *);

void * accept_thread(void *);

void * receive_peer_thread(void * arg);

void receive_active_peer_list(struct peer_list **, const unsigned);

void receive_peer_info(struct peer_list **, const unsigned);

int get_peer_status(const struct peer_list *);

void get_connected_to_last_peer(struct peer_list **);

int init_peer(const struct sockaddr_in *, const in_port_t);

#endif
