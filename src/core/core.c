//TODO: Necessario! Troviamo un'alternativa a realpath() che a quanto pare e' un estensione di GNU e quindi non presente in tutte le implementazioni... RIFERIMENTO : https://sourceforge.net/p/mingw/bugs/2207/
//TODO: Dopo aver tolto realpath, valutiamo se settare una flag per quando l'utente inserisce cartella di download o share sbagliate, come abbiamo fatto con ip e porte, per gestire help
#define _GNU_SOURCE
// <<<<<<<<<<<<<<<<

#include "peer.h"
#include "server.h"
#include "util.h"
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

char download_dir_path[PATH_MAX] = {'\0'};
char share_dir_path[PATH_MAX] = {'\0'};

int main(int argc, char ** argv)
{
  int exit_status = EXIT_SUCCESS;

  static char short_options[] = "SCs:d:i:p:l:hv";
  static struct flags flag = {
    .share_dir = 0,
    .download_dir = 0,
    .server = 0,
    .client = 0,
    .ip = 0,
    .wrong_ip = 0,
    .port = 0,
    .wrong_port = 0,
    .listen_port = 0,
    .wrong_listen_port = 0
  };

  if(argc == 1)
  {
    get_help(NULL);
  }
  else
  {
    short cur_opt = -1;
    struct sockaddr_in sin = {
      .sin_family = AF_INET,
      .sin_addr = {
        .s_addr = htonl(INADDR_ANY)
      }
    };
    in_port_t peer_listen_port = 0;

    memset(&sin.sin_port, 0, sizeof sin.sin_port);

    while((cur_opt = getopt(argc, argv, short_options)) != -1 && exit_status != EXIT_FAILURE)
    {
      switch(cur_opt)
      {
        case 'h':
          get_help(NULL);
          break;

        case 'v':
          get_version();
          break;

        case 'S':
          flag.server = 1;

          if(flag.client == 0)
          {
            //printf("settato flag server\n");
          }
          else
          {
            exit_status = EXIT_FAILURE;
          }

          break;

        case 'C':
          flag.client = 1;

          if(flag.server == 0)
          {
            //printf("settato flag client\n");
          }
          else
          {
            exit_status = EXIT_FAILURE;
          }

          break;

        case 's':
          if(flag.client)
          {
            if(realpath(optarg, share_dir_path) != NULL)
            {
              flag.share_dir = 1;
              //printf("settato flag share\n");
            }
            else
            {
              perror("SHARE_DIR_PATH realpath():");
            }
          }
          else
          {
            exit_status = EXIT_FAILURE;
          }
          break;

        case 'd':
          if(flag.client)
          {
            if(realpath(optarg, download_dir_path) != NULL)
            {
              flag.download_dir = 1;
              //printf("settato flag download\n");

            }
            else
            {
              perror("DOWNLOAD_DIR_PATH realpath():");
            }
          }
          else
          {
            exit_status = EXIT_FAILURE;
          }
          break;

        case 'i':
          if(flag.client)
          {
            if(is_valid_ip(optarg, &sin) != 1)
            {
              flag.wrong_ip = 1;
              exit_status = EXIT_FAILURE;
            }
            else
            {
              flag.ip = 1;
            }
          }
          else
          {
            exit_status = EXIT_FAILURE;
          }
          break;

        case 'p':
          if(is_valid_port(optarg, &sin) != 1)
          {
            flag.wrong_port = 1;
            exit_status = EXIT_FAILURE;
          }
          else
          {
            flag.port = 1;
            //printf("settato flag port\n");
          }
          break;

        case 'l':
          if(is_valid_peer_port(optarg, &peer_listen_port) != 1)
          {
            flag.wrong_listen_port = 1;
            exit_status = EXIT_FAILURE;
          }
          else
          {
            flag.listen_port = 1;
            //printf("settato flag listen_port\n");
          }

          break;

        default:
          exit(EXIT_FAILURE);
          break;
      }
    }

    if(exit_status != EXIT_FAILURE)
    {
      if(flag.client && flag.ip && flag.port && flag.share_dir && flag.download_dir && flag.listen_port)
      {
        //printf("Partiamo con il client!\n");
        exit_status = init_peer(&sin, peer_listen_port);
      }
      else if(flag.server && flag.port)
      {
        //printf("Partiamo con il server!\n");
        exit_status = init_server(&sin);
      }
      else
      {
        get_help(&flag);
      }
    }
    else
    {
      get_help(&flag);
    }
  }

  return exit_status;
}
