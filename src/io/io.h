#ifndef IO_H
#define IO_H

#ifndef BUFSIZE
#define BUFSIZE 100
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include <sys/types.h>
#include <netinet/in.h>

#include <pthread.h>

extern char share_dir_path[PATH_MAX];
extern char download_dir_path[PATH_MAX];

extern pthread_mutex_t download_list_mutex;
extern pthread_mutex_t count_upload_mutex;

struct download_list;

void read_file(struct download_list ** active_download_list, const unsigned upload_peer_fd, int file_size, const char * down_filepath, const char * filename, const in_addr_t peer_addr, const in_port_t peer_port);

void write_file(const unsigned, const char *, int *);

#endif
