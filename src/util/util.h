#ifndef UTIL_H
#define UTIL_H

#include <netinet/in.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif


#ifndef PORT_LENGTH
#define PORT_LENGTH 5
#endif

#ifndef MIN_PORT
#define MIN_PORT 1025
#endif

#ifndef MAX_PORT
#define MAX_PORT 49152
#endif

extern char share_dir_path[PATH_MAX];
extern char download_dir_path[PATH_MAX];

struct flags
{
  unsigned share_dir : 1;
  unsigned download_dir : 1;
  unsigned server : 1;
  unsigned client : 1;
  unsigned ip : 1;
  unsigned wrong_ip : 1;
  unsigned port : 1;
  unsigned wrong_port : 1;
  unsigned listen_port : 1;
  unsigned wrong_listen_port : 1;
};

int is_valid_ip(const char *, struct sockaddr_in *);

int is_valid_port(const char *, struct sockaddr_in *);

int is_valid_peer_port(const char *, in_port_t *);

/**
 * Funzione per controllare l'esistenza di una directory
 * @return        [0 = errore, 1 = successo]
 */
int dir_exist(const char *);

/**
* Funzione per controllare l'esistenza di un file
* @return        [0 = errore, 1 = successo]
*/
off_t file_exist(const char *, char **);

/**
 * Funzione per la ricerca ricorsiva di un file nella directory specificata (e sue sottodirectory)
 */
void search_for_file_in_dir(char *, const char *, char **);

/**
 * Funzione per controllare se un file/directory sono readable per l'EUID del processo corrente
 * @return        [0 = errore, 1 = successo]
 */
int is_readable(const char *);

/**
 * Funzione per controllare se un file/directory sono writable per l'EUID del processo corrente
 * @return        [0 = errore, 1 = successo]
 */
int is_writable(const char *);

/**
 * Funzione per controllare se un file/directory ha la flag X settata per l'EUID del processo corrente
 * @return        [0 = errore, 1 = successo]
 */
int is_executable(const char *);

void get_help(const struct flags *);

void get_version(void);

#endif
