#include <bits/pthreadtypes.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void togli_forchette();
void SigIntHandler(int iSignalCode);
void filosofo(int i, sem_t *forchetta[], int n_filosofi);

int n_filosofi;
sem_t *forchetta[3];
int i;
char nome[20];

struct sigaction sa;

int fd[2];
int cont_stallo;

// Inizializzazione dei flags
int f_stallo = 0;
int f_starv = 0;
int f_sol = 0;

void SigIntHandler(int iSignalCode) {
  //printf("Handler: ricevuto signal %d. %s\n.", iSignalCode,strsignal(iSignalCode));
  togli_forchette(); //rimuove le forchette (semafori)
  return;
}

void SigAlarmHandler(int iSignalCode) {
    printf("RILEVATO DEADLOCK\n");
    exit(0);
}


void mangia(int i /*identificatore del filosofo*/, sem_t *forchetta[], int n_filosofi){
    struct timespec tempo;
    tempo.tv_sec = 0;     // secondi
    tempo.tv_nsec = 8; // nanosecondi 
    
    //signal(SIGALRM, gestore_segnale);
    //sa.sa_handler = SigAlarmHandler;
    //sigaction(SIGALRM, &sa, NULL);

    printf("Filosofo %d, ATTENDO la forchetta destra %d\n", i, i);
    sem_wait(forchetta[i]);
    printf("Filosofo %d, PRENDO la forchetta destra %d\n", i, i);
    printf("Filosofo %d, ATTENDO la forchetta sinistra %d\n", i, (i+1)%n_filosofi);
    sem_wait(forchetta[(i+1)%n_filosofi]);
    printf("Filosofo %d, PRENDO la forchetta sinistra %d\n", i, (i+1)%n_filosofi);

    
    printf("Filosofo %d: mangio...\n", i);
    
    if (nanosleep(&tempo, NULL) == -1) {
      perror("nanosleep");
      exit(EXIT_FAILURE);
    }
    return;
}

void mangia_con_stallo(int i /*identificatore del filosofo*/, sem_t *forchetta[], int n_filosofi){
    struct timespec tempo;
    tempo.tv_sec = 0;     // secondi
    tempo.tv_nsec = 8; // nanosecondi 

    printf("Filosofo %d, ATTENDO la forchetta destra %d\n", i, i);
    sem_wait(forchetta[i]);
    printf("Filosofo %d, PRENDO la forchetta destra %d\n", i, i);
    printf("Filosofo %d, ATTENDO la forchetta sinistra %d\n", i, (i+1)%n_filosofi);
    sem_wait(forchetta[(i+1)%n_filosofi]);
    printf("Filosofo %d, PRENDO la forchetta sinistra %d\n", i, (i+1)%n_filosofi);
    
    read(fd[0], &cont_stallo, sizeof(int));
    cont_stallo++;
    write(fd[1], &cont_stallo, sizeof(int));
    if(cont_stallo == n_filosofi){
        printf("STALLO RILEVATO\n");
        kill(0, SIGINT); //uccido i processi
    }

    
    printf("Filosofo %d: mangio...\n", i);
    
    if (nanosleep(&tempo, NULL) == -1) {
      perror("nanosleep");
      exit(EXIT_FAILURE);
    }
    return;
}

void lascia_forchetta(int i /*identificatore del filosofo*/, sem_t *forchetta[], int n_filosofi){
    printf("Filosofo %d, RILASCIO la forchetta %d\n", i, i);
    sem_post(forchetta[i]);
    printf("Filosofo %d, RILASCIO la forchetta %d\n", i, (i+1)%n_filosofi);
    sem_post(forchetta[(i+1)%n_filosofi]);
    
    return;
}

void filosofo(int i /*identificatore del filosofo*/, sem_t *forchetta[], int n_filosofi) {
  while (1) {
      
    if (f_stallo != 0){ //castistica di rilevazione stallo 
        mangia_con_stallo(i, forchetta, n_filosofi);
    }
    mangia(i, forchetta, n_filosofi);

    lascia_forchetta(i, forchetta, n_filosofi);
    printf("Filosofo %d: ho finito di mangiare\n", i);
    
  }
  
}

int main(int argc, char *argv[]) {

  // segnali
  memset(&sa, '\0', sizeof(struct sigaction));
  sa.sa_handler = SigIntHandler;
  sigaction(SIGINT, &sa, NULL);


  if (argc <= 2) { // perchè il primo è il nome dell'eseguibile e il secondo è il primo argomento
    printf("Numero di filosofi a cena -> %s\n\n\n", argv[1]);
  }

  n_filosofi = atoi(argv[1]);

  if (n_filosofi < 2) {
    printf("pochi filosofi a cena\n");
  }


  if (argv[2] != NULL)
    f_stallo = atoi(argv[2]);
  if (argv[3] != NULL)
    f_sol = atoi(argv[3]);
  if (argv[4] != NULL)
    f_starv = atoi(argv[4]);

  if (f_stallo != 0)
    printf("Flag stallo attivato\n");
  if (f_sol != 0)
    printf("Flag soluzione stallo attivato\n");
  if (f_starv != 0)
    printf("Flag starvation attivato\n");


  //creazione delle forchette
  for (i = 0; i < n_filosofi; i++) {
    
    sprintf(nome, "%d", i);
    
    if ((forchetta[i] = sem_open(nome, O_CREAT, S_IRWXU, 1)) == SEM_FAILED) {
      printf("Errore in sem_open, errno = %d\n", errno);
      exit(EXIT_FAILURE);
    }
  }
  
  pid_t filosofi[n_filosofi]; //gruppo di filosofi a cena
  
  
  //CREAZIONE PIPE PER LA RILEVAZIONE DI STALLO 
  cont_stallo = 0;
  if (pipe(fd) == -1){
    perror("pipe");
    exit(EXIT_FAILURE);
  }
  write(fd[1], &cont_stallo, sizeof(int));
  

  //creazione dei filosofi
  for (i = 0; i < n_filosofi; i++) {
    filosofi[i] = fork();

    if (filosofi[i] == -1) {
      perror("Errore in fork\n");
      exit(EXIT_FAILURE);

    } else if (filosofi[i] == 0) {
      // child
      printf("Sono il filosofo numero: %d\n", i);
      filosofo(i, forchetta, n_filosofi);
      exit(0); //terminazione processo
    } else {
        
    }
  }
  
    for (i = 0; i < n_filosofi; i++){
        wait(NULL);
    }
    
    togli_forchette(); //metodo di rimozione delle forchette (semafori)
    
    //Messaggi signal (per evitare che venga chiamato esmplicitamente tante volte quanti sono i processi)
    printf("Handler: ricevuto signal di Interrupt\n");
    printf("Tolte le %d forchette dalla tavola\n", n_filosofi);
    printf("Tutti i %d filosofi se ne sono andati\n", n_filosofi);

    return 0;   
}

void togli_forchette(){
    for (int i = 0; i < n_filosofi; i++){
        sem_close(forchetta[i]);
        sem_unlink(nome);
    }
    return;
}
