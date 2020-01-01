#include "util.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>

// TODO: Da rimuovere?
#include <limits.h>
#include <sys/types.h>
// <<<<<<<<<<<<<<<<<<<

int is_valid_ip(const char * ip, struct sockaddr_in * sin)
{
  return inet_pton(AF_INET, ip, &(sin -> sin_addr));
}

int is_valid_port(const char * port_str, struct sockaddr_in * sin)
{
  int exit_status = 1;

  int port = strtol(port_str, NULL, 10);

  if(port >= MIN_PORT && port <= MAX_PORT)
  {
    sin -> sin_port = htons(port);
  }
  else
  {
    exit_status = 0;
  }

  return exit_status;
}

int is_valid_peer_port(const char * port_str, in_port_t * peer_listen_port)
{
  int exit_status = 1;

  int port = strtol(port_str, NULL, 10);

  if(port >= MIN_PORT && port <= MAX_PORT)
  {
    *peer_listen_port = htons(port);
  }
  else
  {
    exit_status = 0;
  }

  return exit_status;
}

/**
 * Funzione per garantire l'esistenza di una directory in corrispondenza del pathname specificato da @path
 * @param  path [fullpathname della directory di cui si desidera sapere se esiste o meno]
 * @return      [0 = valore di ritorno in caso di fallimento della system call 'stat' oppure qualora questa rilevi un tipo di dato differente rispetto a quello atteso di directory;
 *               1 = la path immessa esiste all'interno del file system e corrisponde proprio ad una directory]
 */
int dir_exist(const char * path)
{
  int exit_status = 0;
  struct stat st;

  if(!stat(path, &st))
  {
    if(S_ISDIR(st.st_mode))
    {
      exit_status = 1;
    }
  }

  return exit_status;
}

/**
 * Funzione per garantire l'esistenza di un file regolare non directory in corrisondenza del pathname specificato da @path
 * @param  path [full pathname del file di cui si desidera sapere se esiste o meno]
 * @return      [0 = valore di ritorno in caso di fallimento della system call 'stat' oppure qualora questa rilevi un tipo di dato differente rispetto a quello atteso di file regolare non directory;
 *               1 = la path immessa esiste all'interno del file system e corrisponde proprio ad un file regolare non directory]
 */
off_t file_exist(const char * filename, char ** file_path)
{
  off_t exit_status = 0;
  struct stat st;

  //printf("ecco la path di share : %s\n", share_dir_path);

  search_for_file_in_dir(share_dir_path, filename, file_path);

  //printf("ecco il file trovato : %s\n", *file_path);

  if(*file_path)
  {
    if(!stat(*file_path, &st))
    {
      if(S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
      {
        exit_status = st.st_size;
      }
    }
  }

  return exit_status;
}

/**
 * Funzione per la ricerca ed il retrieve del full pathname di @filename nella directory specificata da @pathname
 * @param path       [pathname (full o relative) della directory in cui si ricercherà @filename]
 * @param filename   [nome del file di cui si desidera ottenere il full pathname]
 * @param found_path [parametro formale in cui sarà memorizzata la path di @filename (se questo è presente in @pathname, altrimenti non ne sarà modificato il valore)]
 */
void search_for_file_in_dir(char * path, const char * filename, char ** found_path)
{
  char pathname[PATH_MAX];
  DIR * dir;

  if((dir = opendir(path)) != NULL)
  {
    struct dirent * d;
    struct stat st;
    char buffer[PATH_MAX];

    while((d = readdir(dir)) != NULL)
    {
      memset(buffer, 0, PATH_MAX);
      sprintf(buffer, "%s/%s", path, d -> d_name);

      if(!stat(buffer, &st))
      {
        if(S_ISREG(st.st_mode))
        {
          if(!strcmp(filename, d -> d_name))
          {
            *found_path = (char *)malloc(PATH_MAX);
            strcpy(*found_path, path);
            sprintf(*found_path, "%s/%s", *found_path, d -> d_name);
            // //printf("ecco il file %s\n", *found_path);
          }
        }
        else if(S_ISDIR(st.st_mode))
        {
          if(strcmp(d -> d_name, ".") && strcmp(d -> d_name, ".."))
          {
            sprintf(pathname, "%s/%s", path, d -> d_name);

            if(*found_path == NULL)
            {
              search_for_file_in_dir(pathname, filename, found_path);
            }
          }
        }
      }
    }

    closedir(dir);
  }

  return;
}

/**
 * Controlla se il processo corrente ha i permessi di accesso in lettura su quanto corrisponde a @path
 * @param  path [full pathname di file o directory di cui si vogliono sfogliare i permessi]
 * @return      [0 = il processo corrente non dispone dei permessi richiesti; 1 = il processo può conseguire l'accesso in lettura]
 */
int is_readable(const char * path)
{
  int exit_status = 0;
  struct stat st;

  if(!stat(path, &st))
  {
    uid_t euid = geteuid();

    if(euid == 0)
    {
      exit_status = 1;
    }
    else
    {
      if(euid == st.st_uid)
      {
        if((st.st_mode & S_IRUSR))
        {
          exit_status = 1;
        }
      }
      else if(getegid() == st.st_gid)
      {
        if((st.st_mode & S_IRGRP))
        {
          exit_status = 1;
        }
      }
      else
      {
        if((st.st_mode & S_IROTH))
        {
          exit_status = 1;
        }
      }
    }
  }

  return exit_status;
}

/**
* Controlla se il processo corrente ha i permessi di accesso in scrittura su quanto corrisponde a @path
* @param  path [full pathname di file o directory di cui si vogliono sfogliare i permessi]
* @return      [0 = il processo corrente non dispone dei permessi richiesti; 1 = il processo può conseguire l'accesso in scrittura]
*/
int is_writable(const char * path)
{
 int exit_status = 0;
 struct stat st;

 if(!stat(path, &st))
 {
   uid_t euid = geteuid();

   if(euid == 0)
   {
     exit_status = 1;
   }
   else
   {
     if(euid == st.st_uid)
     {
       if((st.st_mode & S_IWUSR))
       {
         exit_status = 1;
       }
     }
     else if(getegid() == st.st_gid)
     {
       if((st.st_mode & S_IWGRP))
       {
         exit_status = 1;
       }
     }
     else
     {
       if((st.st_mode & S_IWOTH))
       {
         exit_status = 1;
       }
     }
   }
 }

 return exit_status;
}

/**
* Controlla se il processo corrente ha i permessi di accesso in esecuzione su quanto corrisponde a @path
* @param  path [full pathname di file o directory di cui si vogliono sfogliare i permessi]
* @return      [0 = il processo corrente non dispone dei permessi richiesti; 1 = il processo può conseguire l'accesso in esecuzione]
*/
int is_executable(const char * path)
{
 int exit_status = 0;
 struct stat st;

 if(!stat(path, &st))
 {
   uid_t euid = geteuid();

   if(euid == 0)
   {
     exit_status = 1;
   }
   else
   {
     if(euid == st.st_uid)
     {
       if((st.st_mode & S_IXUSR))
       {
         exit_status = 1;
       }
     }
     else if(getegid() == st.st_gid)
     {
       if((st.st_mode & S_IXGRP))
       {
         exit_status = 1;
       }
     }
     else
     {
       if((st.st_mode & S_IXOTH))
       {
         exit_status = 1;
       }
     }
   }
 }

 return exit_status;
}

void get_help(const struct flags * flag)
{
  if(flag)
  {
    if(flag -> server && flag -> client)
    {
      //printf("O avvii il server o il peer!!\n");
    }
    else
    {
      if(flag -> client)
      {
        if(!flag -> share_dir)
        {
          //printf("Il client ha bisgono di una directory di share!\n");
        }

        if(!flag -> download_dir)
        {
          //printf("Il client ha bisogno di una directory di download!\n");
        }

        if(!flag -> ip)
        {
          if(flag -> wrong_ip)
          {
            //printf("Attenzione, l'ip immesso non e' valido\n");
          }
          else
          {
            //printf("Il client ha bisogno di sapere l'ip del server di rendez-vous\n");
          }
        }

        if(!flag -> port)
        {
          if(flag -> wrong_port)
          {
            //printf("la porta di server immessa non e' valida\n");
          }
          else
          {
            //printf("Client necessita di porta del server\n");
          }
        }

        if(!flag -> listen_port)
        {
          if(flag -> wrong_listen_port)
          {
            //printf("Porta di ascolto non valida\n");
          }
          else
          {
            //printf("Inserire la porta su cui il client si mettera in ascolto\n");
          }
        }
      }
      else
      {
        if(!flag -> port)
        {
          if(flag -> wrong_port)
          {
            //printf("la porta di server immessa non e' valida\n");
          }
          else
          {
            //printf("Il server ha bisogno di sapere su quale porta mettersi in ascolto.");
          }
        }
      }
    }
  }
  else
  {
    static const char * const usage =
    "\nSFS e' stato creato nell'ambito del Progetto relativo all'esame di Laboratorio di Sistemi Operativi ed implementa un servizio distribuito di file sharing peer-to-peer.\n\n"
    "Uso: sfs -C -i IP -p PORT -l PORT -s DIR -d DIR\n"
    "     sfs -s DIR\n"
    "     sfs -d DIR\n"
    "     sfs -S -l\n"
    "\n"
    "  -h\t\t\t\tMostra questo aiuto\n"
    "  -s\tDIR\t\tImposta DIR come directory di condivisione.\n"
    "  -d\tDIR\t\tImposta DIR come directory di download.\n"
    "  -S\t\t\t\tAvvia un'istanza di SFS in modalita' di server di rendez-vous.\n"
    "  -C\t\t\t\tAvvia un'istanza di SFS in modalita' di peer.\n"
    "  -l\tPORT\t\tLa porta su cui il peer si mette in ascolto.\n"
    "  -i\t\tIP\t\tIndica l'IPv4 e la porta del server di rendez-vous a cui collegarsi.\n"
    "  -p\t\tPORT\t\tLa porta del server di rendez-vous"
    "  -v\t\t\t\tVisualizza la versione del programma.\n\n";
    // write(STDOUT_FILENO, usage, strlen(usage));

    //printf("%s\n", usage);
  }

  return;
}

void get_version(void)
{
  return;
}
