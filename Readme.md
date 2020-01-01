README v1.0 / 01 JAN 2018

# SFS - Simple File Sharing

## Introduzione

__SFS__ e' stato creato nell'ambito del Progetto relativo all'esame di _Laboratorio di Sistemi Operativi_ ed implementa un servizio distribuito di file sharing _peer-to-peer_.

## Utilizzo

|Opzione|Argomento|Descrizione|
|:----------:|:-------:|-----------|
|-h| |Visualizza le varie opzioni e come utilizzarle.|
|-s|__DIR__|Imposta __DIR__ come directory di condivisione.|
|-d|__DIR__|Imposta __DIR__ come directory di download.|
|-S||Avvia un'istanza di __SFS__ in modalita' server di rendez-vous.|
|-C||Avvia un'istanza di __SFS__ in modalita' peer.|
|-l|__PORT__|La porta su cui il peer si mette in ascolto.|
|-i|__IP__|L'__IPv4__ del server di _rendez-vous_|
|-p|__PORT__|La __PORT__ del server di _rendez-vous_|
|-v||Visualizza la versione del programma.||

<div style="page-break-after: always;"></div>

### Esempi di uso

#### Server di rendez-vous
__SFS__ puo' essere configurato in modo tale da agire come server di _rendez-vous_, in attesa di connessioni sulla porta `5200`, nel seguente modo:

`$ sfs -S -p 5200`

---

#### Connessione ad un server di rendez-vous

Per collegarsi ad un server di _rendez-vous_ e' necessario specificare l'IPv4 e la porta di quest'ultimo e la porta su cui il peer si mettera' in ascolto:


`$ sfs -C  -i 143.225.172.146 -p 5200 -l 7000 [...]`

---

#### Specificare le directory di download e di condivisione

E' possibile specificare una directory di `download` ed una di `condivisione`, che verranno utilizzate successivamente da __SFS__ nel caso in cui lo si avvii in modalita' peer:

`$ sfs [...] -d /home/claudio/Documenti -s ~/Scaricati`

\* _Sono accettate sia path relative che assolute_

<div style="page-break-after: always;"></div>

## Compilazione dei sorgenti

### Prerequisiti

I prerequisiti per la compilazione dei sorgenti di __SFS__ sono:
* __make__
* Qualsiasi compilatore che rispetta gli standard __C99__

__N.B:__ Il compilatore utilizzato in fase di progettazione e' __GCC__ versione __7.2.1__

### Dipendenze

__SFS__ utilizza librerie standard del _C_ e _syscall_ _Unix/Linux_ __POSIX__ _compliant_.

Per questo motivo non esistono dipendenze particolari per la compilazione.

## Installazione

__SFS__ viene fornito in bundle con un comodo __Makefile__.

Per procedere all'installazione di __SFS__ sul proprio PC e' necessario assumere i privilegi di amministratore ed eseguire `make install`.

Se cio' non dovesse essere possibile per qualsivoglia motivo e' possibile utilizzare il software invocando `make sfs`.

__N.B:__ Utilizzando il secondo metodo non sara' possibile invocare __SFS__ se non all'interno della directory `bin/sfs`.

Per ovviare a cio' puo'__SFS__ e' stato creato nell'ambito del Progetto relativo all'esame di _Laboratorio di Sistemi Operativi_ ed implementa un servizio distribuito di file sharing _peer-to-peer_. essere aggiornata la variabile di ambiente `PATH`. In particolare, cio' e' possibile eseguendo un export della path `bin/sfs`, mantenendo quindi tale modifica locale alla sessione attuale della shell corrente; viceversa, per rendere la modifica permanente, e' necessario appendere la path alla variabile d'ambiente `PATH` nel file `~/.profile` (oppure `~/bash_profile`) ed effettuare una riconnessione.

<div style="page-break-after: always;"></div>

## Autori

__SFS__ e' stato progettato ed implementato da

* Claudio Cortese - N86001886
* Emilia Del Re - N86001966
