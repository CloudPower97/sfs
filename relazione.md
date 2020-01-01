<div style="text-align: center;">
  <h1>
    Progetto di Laboratorio di Sistemi Operativi
  </h1>

  <h2>
    Claudio Cortese - N86001886  
  </h2>

  <h2>
    Emilia Del Re - N86001966
  </h2>

  <h3>
    <span style="font-weight: bold">S</span>imple <span style="font-weight: bold">F</span>ile <span style="font-weight: bold">S</span>haring (<span style="font-weight: bold">SFS</span>)
  </h3>
</div>


<div style="page-break-after: always;"></div>

## Descrizione

__SFS__ e' stato creato nell'ambito del Progetto relativo all'esame di _Laboratorio di Sistemi Operativi_ ed implementa un servizio distribuito di file sharing __peer-to-peer__.

__SFS__ e' un'applicativo in grado di essere eseguito sia in modalita' di server di _rendez-vous_ che in modalita' __peer__.

## Guida d'Uso

Tutte le informazioni riguardo la compilazione di __SFS__ e l'utilizzo di quest'ultimo, compreso esempi illustrativi, sono presenti nel file _Readme.pdf_ all'interno della cartella __doc/__.

<div style="page-break-after: always;"></div>

## Protocollo

In questa sezione si vuole descrivere, analizzandolo, il protocollo adottato all'interno di __SFS__ per permettere il corretto funzionamento di quest'ultimo.

Tutti gli scambi di informazioni vengono veicolati attraverso il protocollo __TCP/IP__.

<h3 style="text-align: center">
  Comunicazioni con il server di rendez-vous
</h3>

#### Notifica di un nuovo _peer_

All'atto della connessione di un _peer_ alla rete, quest'ultimo invia le informazioni riguardanti l'__IP__ e la porta, su cui avverranno le comunicazioni con gli altri _peer_, al server di __.

Piu' propriamente, l'indirizzo __IP__ (__IPv4__) e' restituito dalla struttura `sockaddr_in`, riempita dalla chiamata alla _system call_ `accept()`, al momento dell'handshake tra il _peer_ appena connesso al suddetto server; sara' solo il numero di porta ad essere inviato tramite la connessione _socket_ stabilita.

L'accettazione del nuovo _peer_ (nonchè la gestione delle comunicazioni con questo) è demandata interamente ad un _thread_ specifico per il client.

Tramite tale _thread_, il server rende disponibili le informazioni riguardanti i _peer_ gia' presenti in rete al nuovo connesso mediante il medesimo canale di comunicazione; una volta ottenute le informazioni e storate opportunamente per mezzo di una lista concatenata, il server di _rendez vous_ le inviera' poi a sua volta a tutti i _peer_ attualmente connessi e, aggiornando di volta in volta, ad i _peer_ futuri.

---

#### Cambio di stato dei _peer_

Quando un _peer_ fa richiesta di terminazione (a seguito di esplicita richiesta utente), questa viene inoltrata al server di , che provvede ad aggiornare le proprie informazioni riguardo i nodi della rete e ad aggiornare conseguentemente i _peer_ interessati : se il _peer_ richiedente ha _download_ od _upload_ in corso, la rimozione definitiva viene posticipata fino all'esaurimento dei rispettivi trasferimenti.

In particolare, ai _peer_ attualmente connessi vengono inviati indirizzo __IP__ e numero di porta del _peer_ richiedente la terminazione.

I _peer_ riceventi si occuperanno, con opportuni controlli, di registrare il cambiamento notificato; piu' in particolare, se questi sono gia' in possesso delle informazioni appena ricevute ed il rispettivo nodo risulta avere stato "_attivo_", questi lo modificheranno in "_in terminazione_".

---

#### Terminazione definitiva del _peer_

Quando un _peer_ puo' disconnettersi definitivamente, il server di _rendez-vous_ provvede ad inviare nuovamente ai _peer_ attualmente connessi le coordinate di networking del _peer_ ormai rimosso dalla propria lista di nodi di rete.

I restanti _peer_ connessi si occupano quindi di rimuovere definitivamente le rispettive informazioni dalla propria lista personale, confrontando nuovamente __IP__ e porta ricevuti con quelli registrati e valutando il rispettivo stato del nodo : se questi risulta gia' "_in terminazione_", allora sara' necessaria la rimozione dei rispettivi dati.

---

#### Comunicazione dei nuovi _peer_ e dei cambi di stato a tutti i _peer_ attivi

<h3 style="text-align: center">
Richieste di download
</h3>

All'atto della richiesta di _download_ da parte dell'utente, viene avviato un _thread_ specifico per la gestione; cio' per' accade se e solo se il _peer_ in questione ha stato ancora attivo; viceversa, le immissioni di nomi di file per il _download_ vengono totalmente ignorate.

La richiesta viene inoltrata ai _peer_ attivi a cui il nodo risulta collegato, eccetto a quelli in terminazione.

La comunicazione avviene su una specifica _socket_/canale di comunicazione creato appositamente, onde evitare overcluttering di file desciptor dovuti a richieste concorrenti.

<h3 style="text-align: center">
  Invio e Ricezione di file
</h3>

#### Invio

All'atto della ricezione della richiesta di _upload_ a seguito di una reciproca di _download_, __SFS__ controlla preventivamente l'esistenza o meno del file richiesto all'interno della cartella di condivisione specificata all'avvio dell'applicazione tramite flag `-s`.

In caso in cui il suddetto file esista viene inviato un 'ok' al _peer_ richiedente, che a sua volta evitera' di inviare richieste agli altri _peer_ della rete.

Si prepara quindi un nuovo canale di comunicazione e si rende disponibile al _peer_ richiedente, tramite la creazione di una _socket_ temporanea sulla prima porta libera indicata dal sistema operativo stesso, da notificare al _peer_, e successivamente si procede all'effettivo _upload_ del file.

---

#### Ricezione

Nel momento in cui uno dei _peer_ informa che il file cercato dall'utente e' disponibile, il _peer_ richiedente il _download_ si connette tramite _socket_ all'_upload peer_ e su questo canale di comunicazione avviene l'effettivo _download_ del file, in modo da garantire che successive richieste a questo specifico _peer_ vengano gestite in modo corretto.

 Il _peer_ che ha richiesto il _download_ apre un nuovo file, omonimo a quello richiesto, nella directory di _download_ specificata all'atto dell'avvio del processo client tramite flag `-d`.
A terminazione del _download_, il file descriptor rispettivo alla _socket_ temporanea viene chiuso.

<!-- <h3 style="text-align: center">
  Terminazione di un _peer_
</h3> -->

<!-- #### Invio della richiesta di terminazione di un _peer_

All'atto della richiesta di terminazione del _peer_, viene contattato il server di rendez-vous; a sua volta, come già detto, questi lo notifica a tutti gli altri.
Il suddetto _peer_ non contribuira' ulteriormente a soddisfare richieste di _download_.

---

#### Effettiva terminazione del _peer_

Il _peer_ rimane in stato di terminazione finche' e in corso un _download_ e/o un _upload_.
Successivamente viene notificato il server di rendez-vous dell'effettiva terminazione del _peer_.
E termina il _peer_. -->

<div style="page-break-after: always;"></div>

## Dettagli Implementativi

In questa sezione si vogliono descrivere i dettagli Implementativi giudicati piu' interessanti.

<h3 style="text-align: center">
  Utilizzo coerente dello snake_case
</h3>

Durante la stesura del codice di __SFS__ si e' fatto utilizzo dello __snake_case__

<h3 style="text-align: center">
  Utilizzo di MakeFile
</h3>

Per aiutarci durante la fase di testing e di deploy dell'applicativo, abbiamo scelto di utilizzare un Makefile, comprensivo di più flag di compilazione e warning :

```Makefile
CFLAGS=-W -Wshadow -Wwrite-strings -ansi -Wcast-qual -Wall -Wextra -Wpedantic -pedantic -std=c99
```

In particolare, si noti l'utilizzo dello standard __C__ in versione __99__, scelto perlopiù per semplificare l'inizializzazione delle strutture.

<h3 style="text-align: center">
  Libreria Statica
</h3>

Si è scelto l'utilizzo di una libreria statica per rendere lo sviluppo più semplice e veloce.

<h3 style="text-align: center">
  Utilizzo di Optarg
</h3>

L'utilizzo della funzione standard `getopt()` ci ha permesso di fornire delle comode `short options` per eseguire in modo corretto il programma, con gli opportuni parametri, unificando inoltre la porzione di codice relativa all'inizializzazione in modalità di _server_ di _rendez vous_ e rispettivamente di _peer_.

<div style="page-break-after: always;"></div>

<h3 style="text-align: center">
  Struttura flags
</h3>

```C
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
```

<h3 style="text-align: center">
  Controllo correttezza flag e passaggio parametri ad `init_peer()`
</h3>

```C
  if(flag.client && flag.ip && flag.port && flag.share_dir && flag._download_dir && flag.listen_port)
  {
    printf("Partiamo con il client!\n");
    exit_status = init_peer(&sin, _peer_listen_port);
  }
```

<h3 style="text-align: center">
  Controllo correttezza flag e passaggio parametri ad `init_server()`
</h3>

```C
if(flag.server && flag.port)
{
  printf("Partiamo con il server!\n");
  exit_status = init_server(&sin);
}
```

<h3 style="text-align: center">
  Invocazione a `get_help()`
</h3>

Viene mostrato all'utente un help, specifico per ogni caso d'uso.
In particolare le invocazioni alle chiamate a `get_help()` accadono quando l'utente utilizza in modo errato le flag con cui avviare il programma.

Esempio di chiamata:

```C
get_help(&flag);
```

---

<h3 style="text-align: center">
  Thread impiegati nell'applicativo e rispettivo ruolo
</h3>

N.B Tutti i _thread_ utilizzati nell'applicazione sono creati in modalità detached, al fine di parallelizzare i compiti da loro svolti.

#### Server di _rendez vous_

Il master _thread_ del server di _rendez vous_ ha il compito principale di creare un canale di comunicazione __TCP/IP__ per accogliere le richieste dei nodi _peer_ della rete; ad ognuno di questi, all'atto della connessione, associa un _thread_ specifico.

```C
void * client_thread(void * arg)

```

È il _thread_ associato dal server di rendez-vous ad ogni client che vi si connette.
Si occupa di gestire tutte le comunicazioni con quest'ultimo, a partire dalla lettura della porta su cui il client si metterà in ascolto per gli altri _peer_ della rete; si occupa inoltre di aggiungere alla struttura dati propria del server di _redez-vous_ le informazioni inerenti ai _peer_ a seguito della loro connessione (__IP__, porta, stato attuale).

Ad ogni _peer_ introdotto nella rete, vengono comunicate le informazioni in merito agli altri nodi connessi :

```C
send_active_peer_list(*client_info -> active_peer_list, client_addr, client_port, client_fd);
```

Inoltre, agli altri _peer_ della rete viene reso noto il nuovo nodo aggiunto :

```C
send_last_connected_client(*client_info -> active_peer_list, client_addr, client_port);
```

Terminata tale fase di inizializzazione, al _thread_ non rimane altro se non attendere la richiesta di modifica di stato da parte del client che questi si occupa di gestire;
abbiamo scelto a tal fine di porre una lettura bloccante :

```C
read(client_fd, stop, sizeof stop);
```

Quando il client avrà notificato al _thread_ rispettivo per la prima volta, ciò sarà esclusivamente per una transizione dallo stato "attivo" allo stato di "in terminazione";
il _thread_ si occuperà quindi di modificare opportunamente la lista condivisa con il master _thread_ :

```C
change_peer_status(*client_info -> active_peer_list, client_addr, client_port, TERMINAZIONE);
```

E di notificarlo a tutti gli altri _peer_ della rete :

```C
send_changed_status_peer_(*client_info -> active_peer_list, client_addr, client_port);
```

Il _thread_ si pone poi in attesa di una successiva notifica da parte di quel client, nuovamente tramite lettura bloccante :

```C
read(client_fd, stop, sizeof stop);
```

Quando sarà uscito da tale lettura, il _thread_ saprà che ciò è dovuto ad una richiesta definitiva di terminazione;
procederà quindi con l'eliminazione del rispettivo nodo dalla propria lista concatenata :

```C
*client_info -> active__peer__list = discard_peer_node(*client_info -> active_peer_list,client_addr, client_port);
```

E conseguentemente notificherà i _peer_ della rete :

```C
send_peer_to_discard(*client_info -> active_peer_list, client_addr, client_port);
```

---

#### Processo _peer_

Il ruolo principale del master _thread_ del processo _peer_ è quello di instaurare la connessione al server di rendez-vous e di creare opportunamente gli altri _thread_, nonchè di fornire il prompt all'utente, corredato di gestione dei comandi.

In particolare, subito dopo la connessione al server, il _peer_ crea uno specifico _thread_ per la parte server della comunicazione _peer-to-peer_.

##### Accept_thread

```C
void * accept_thread(void * arg)
```

Il suddetto _thread_ si occupa di creare un canale di comunicazione su cui si metterà in ascolto degli altri _peer_ attivi.
Dopo ogni connessione stabilita con un _peer_, tale _thread_ si occupa di creare un ulteriore _thread_, apposito per la gestione delle richieste di _upload_ provenienti dal suddetto _peer_.

#### Handle_upload_thread

```C
void * handle_upload_thread(void * arg)
```

Questo _thread_ si occupa, in un while(1), di attendere richieste di _upload_ da parte degli altri _peer_ attivi nella rete.

Più in particolare, questi attende l'invio del nome di file di cui si desidera effettuare il _download_ e rimane in lettura bloccante della lunghezza di suddetto nome e del nome stesso (ciò consentirà alla successiva lettura di sapere esattamente quanti byte attendersi) :

```C
read(peer_fd, &filename_len, sizeof filename_len);
read(peer_fd, filename, filename_len);
```

Quando sarà uscito dalle due letture, il _thread_ saprà di dover gestire la ricerca del file ed un suo possibile _upload_; di conseguenza, si predispone alla creazione di un nuovo canale di comunicazione, che si occuperà esclusivamente del trasferimento del file.

A tal fine, si occupa della parte server di tale canale, legando il file descriptor alla prima porta disponibile fornita dal sistema operativo, ed attendendosi connessioni su qualunque interfaccia di rete della macchina corrente; comunicherà poi la porta scelta al _peer_, tramite il file descriptor associato precedentemente dall'`accept_thread` :

```C
if(!getsockname(sockfd, (struct sockaddr *)&sin, &len))
{
  write(peer_fd, &sin.sin_port, sizeof sin.sin_port);
}

...

else
{
  int flag = -1;

  write(peer_fd, &flag, sizeof flag);
}
```

In tal modo ci assicuriamo, in ogni caso, di scrivere qualcosa al _peer_ che attende la lettura, e che riceverà quindi un valore di discrimine per sapere se connettersi o meno.

Se l'assegnazione di una porta al _socket_ appena creato ha successo, si procede con l'instaurare una connessione con il _peer_ richiedente.
Un _thread_ opportuno gestirà SOLO QUESTA richiesta.


#### upload_thread

Al suddetto _thread_ si demanda il compito di cercare il file richiesto all'interno della propria directory di share; inoltre, se questo viene trovato (tramite ricerca ricorsiva, nella directory specificata e in eventuali sue sottodirectory) il _thread_ comunica al _download thread_ del _peer_ rispettivo la dimensione del file da attendere, 0 altrimenti.

Si procederà poi con il trasferimento del file se questi è presente e, ad ogni _upload_ che questo _thread_ effettua, si incrementerà la variabile che conteggia il numero di _upload_ in corso.

```C
try_upload_file_to_one_peer(u_info.peer_fd, u_info.filename, &count_upload);
```

Al termine di ciò, il controllo che segue si occuperà di controllare se è stata nel frattempo notificata una richiesta di stop per il _peer_ corrente :

```C
if(got_stop == 1)
{
  raise(SIGUSR1);
}
```

Il signal handler seguente si occupa della rispettiva gestione del segnale :

```C
void sig_handler(int signal)
{
  if(signal == SIGUSR1)
  {
    got_stop = 2;

    pthread_mutex_lock(&download_list_mutex);
    active_download_list = free_download_list(active_download_list);
    pthread_mutex_unlock(&download_list_mutex);

    pthread_mutex_lock(&_peer_list_mutex);
    active_peer_list = free_peer_list(active_peer_list);
    pthread_mutex_unlock(&peer_list_mutex);

    raise(SIGINT);
  }
}
```

#### Receive_peer_thread

È il _thread_ che si occupa della fase di inizializzazione lato _peer_ :

```C
write(info.serv_fd, &info.peer_listen_port, sizeof info.peer_listen_port);
```

Il _peer_ invia la porta su cui attende connessioni dagli altri nodi della rete; dopodichè, si aspetta di ricevere la lista di _peer_ attivi da parte del server di _rendez vous_ :

```C
receive_active_peer_list(info.active_peer_list, info.serv_fd);
```

<div style="page-break-after: always;"></div>

E rimane poi in attesa di ricevere le informazioni riguardanti ogni _peer_ che si connetterà successivamente :

```C
while(1)
{
  receive__peer__info(info.active__peer__list, info.serv_fd);
}
```

#### download_thread

Il _thread_ di cui sopra si occupa strettamente della propagazione della richiesta di _download_ ai _peer_ attivi, fin quando no se ne trova uno disponibile all'_upload_ delle informazioni richieste.

Per ogni _peer_ a cui giunge la richiesta di _download_ (espressa tramite invio del filename) il _thread_ attende la ricezione di un intero (off_t per la precisione) che farà da discrimine; se maggiore di zero, indica la dimensione del file trovato.
Il _thread_ si predispone quindi alla ricezione del file da quel _peer_, evitando quindi le richieste ai nodi successivi.

```C
_download__peer__info = try__download_file_from_any__peer_(d_info.active_peer__list, d_info.active_download_list, d_info.filename);
```

La funzione di cui sopra si occupa inoltre di riempire la lista concatenata dei _download_ attualmente in corso e di modificare i nodi che subiscono variazioni (avanzamento del _download_).

La rimozione del nodo di _download_ è appannaggio del _thread_ all'uscita dalla funzione, che poi controlla se c'è stata l'immissione di uno stop da parte dell'utente :

```C
if(got_stop == 1)
{
  pthread_mutex_lock(&count_upload_mutex);

  if((_download__count = count_download(*d_info.active_download_list)) == 0 && count_upload == 0)
  {
    raise(SIGUSR1);
  }

  pthread_mutex_unlock(&count_upload_mutex);
}
```

Si controlla infatti se c'è stato uno stop, e si vede se non ci sono n'è _download_ nè _upload_ in corso; in tal caso, la richiesta di terminazione è accordata immediatamente e il segnale è gestito a tal fine dall'handler di cui sopra.
