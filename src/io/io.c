#include "io.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include <netinet/in.h>
#include <pthread.h>
#include "network.h"
#include "peer.h"

struct download_list;

void read_file(struct download_list ** active_download_list, const unsigned upload_peer_fd, int file_size, const char * down_filepath, const char * filename, const in_addr_t peer_addr, const in_port_t peer_port)
{
  int file_fd = creat(down_filepath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

  if(file_fd > 0)
  {
    ssize_t actually_read = 0;

    char buffer[BUFSIZE] = {'\0'};

    ssize_t tmp = 0;

    do
    {
      if((actually_read = read(upload_peer_fd, buffer, BUFSIZE)) > 0)
      {
        file_size -= actually_read;

        ssize_t totally_written = 0;
        ssize_t actually_written = 0;

        do
        {
          actually_written = write(file_fd, buffer + totally_written, actually_read - totally_written);

          if(actually_written == -1)
          {
            perror("write()");
          }
          else
          {
            totally_written += actually_written;

            tmp += actually_written;

            // pthread_mutex_lock(&download_list_mutex);
            update_download_info(*active_download_list, filename, peer_addr, peer_port, tmp);
            // pthread_mutex_unlock(&download_list_mutex);
          }
        }
        while(totally_written < actually_read && actually_written != -1);

        memset(buffer, 0, BUFSIZE);
      }
      else
      {
        perror("read()");
      }
    }
    while(actually_read != -1 && file_size != 0);

    close(file_fd);
  }
  else
  {
    perror("creat()");
  }

  return;
}

void write_file(const unsigned download_peer_fd, const char * up_filepath, int * count_upload)
{
  pthread_mutex_lock(&count_upload_mutex);

  *count_upload = *count_upload + 1;

  pthread_mutex_unlock(&count_upload_mutex);

  int file_fd = open(up_filepath, O_RDONLY);


  if(file_fd != -1)
  {
    char buffer[BUFSIZE] = {0};
    ssize_t actually_read = 1;

    while(actually_read > 0)
    {
      actually_read = read(file_fd, buffer, BUFSIZE);

      ssize_t totally_written = 0;
      ssize_t actually_written = 0;

      do
      {
        actually_written = write(download_peer_fd, buffer + totally_written, actually_read - totally_written);

        if(actually_written == -1)
        {
          perror("write()");
        }
        else
        {
          totally_written += actually_written;
        }
      }
      while(totally_written < actually_read && actually_written != -1);

      memset(buffer, 0, BUFSIZE);
      sleep(1);

    }

    close(file_fd);
  }
  else
  {
    perror("open()");
  }

  return;
}
