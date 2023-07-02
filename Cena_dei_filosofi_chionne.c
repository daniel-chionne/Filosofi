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
void filosofo(int i, sem_t *forchetta[]);


int n_filosofi;
sem_t *forchetta[3];
char nome[20][10];

struct sigaction sa;
struct timespec tempo;

int fd[2];
int cont_stallo;

// Inizializzazione dei flags
int f_stallo;
int f_starv;
int f_sol;


void SigIntHandler(int iSignalCode) {
  //printf("Handler: ricevuto signal %d. %s\n.", iSignalCode,strsignal(iSignalCode));
  togli_forchette(); //rimuove le forchette (semafori)
  return;
}

//FUNZIONE COMPORTAMENTO FILOSOFO
void filosofo(int i /*identificatore del filosofo*/, sem_t *forchetta[]) {

  int destra = i;
  int sinistra = (i+1)%n_filosofi;

  //SOLUZIONE ALLO STALLO
  if (f_sol != 0){
    if (i == n_filosofi - 1){
      // l'ultimo filosofo inverte l’ordine di presa delle forchette
      destra = (i+1)%n_filosofi;
      sinistra = i;
    }
  }

  while (1) {
    tempo.tv_sec = 0;   // secondi
    tempo.tv_nsec = 8; // nanosecondi 

    printf("Filosofo %d, ATTENDO la forchetta %d\n", i, destra);
    sem_wait(forchetta[destra]);
    printf("Filosofo %d, PRENDO la forchetta %d\n", i, destra);
    fflush(stdout);

    if (f_stallo != 0) sleep(1); //favorisce lo stallo

    printf("Filosofo %d, ATTENDO la forchetta %d\n", i, sinistra);

    if (f_stallo != 0){
      read(fd[0], &cont_stallo, sizeof(int));
      cont_stallo++;
      write(fd[1], &cont_stallo, sizeof(int));
      if(cont_stallo == n_filosofi){
        fprintf(stderr,"STALLO RILEVATO\n");
        kill(0, SIGINT); //uccido i processi
      }
    }
    
    sem_wait(forchetta[sinistra]);
    printf("Filosofo %d, PRENDO la forchetta %d\n", i, sinistra);
    read(fd[0], &cont_stallo, sizeof(int));

    //prevenzione dello stallo -> se riesco a prendere la forchetta sinistra decremento il contatore 
    if (f_stallo != 0){
      cont_stallo--;
      write(fd[1], &cont_stallo, sizeof(int));
      printf("COUNTER %d\n", cont_stallo);
    }
    
    printf("Filosofo %d: mangio...\n", i);
    nanosleep(&tempo, NULL);

    sem_post(forchetta[destra]);
    printf("Filosofo %d, RILASCIO la forchetta %d\n", i, destra);
    sem_post(forchetta[sinistra]);
    printf("Filosofo %d, RILASCIO la forchetta %d\n", i, sinistra);    
  }
}

void togli_forchette(){
    for (int i = 0; i < n_filosofi; i++){
        sem_close(forchetta[i]);
        sem_unlink(nome[i]);
    }
    return;
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
    printf("Pochi filosofi a cena. Ce ne devono essere almeno 2\n");
    return EXIT_FAILURE;
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
  for (int i = 0; i < n_filosofi; i++) {
    
    sprintf(nome[i], "%d", i);
    
    if ((forchetta[i] = sem_open(nome[i], O_CREAT, S_IRWXU, 1)) == SEM_FAILED) {
      printf("Errore in sem_open, errno = %d\n", errno);
      exit(EXIT_FAILURE);
    }
  }
  
  pid_t filosofi[n_filosofi]; //gruppo di filosofi a cena
  
  //CREAZIONE PIPE PER LA RILEVAZIONE DI STALLO
  if (f_stallo != 0){
    cont_stallo = 0;
    if (pipe(fd) == -1){
      perror("pipe");
      exit(EXIT_FAILURE);
    }
    write(fd[1], &cont_stallo, sizeof(int));
  }
  
    
  //creazione dei filosofi mediante processi figli
  for (int i = 0; i < n_filosofi; i++) {
    filosofi[i] = fork();

    if (filosofi[i] == -1) {
      perror("Errore in fork\n");
      exit(EXIT_FAILURE);

    } else if (filosofi[i] == 0) {
      // child
      printf("Sono il filosofo numero: %d\n", i);
      filosofo(i, forchetta);
      exit(0); //terminazione processo
    }
  }
  
  //il padre sta in attesa dei filosofi
  for (int i = 0; i < n_filosofi; i++){
      waitpid(filosofi[i], NULL, 0);
  }
    
  printf("Capo: Fine della cena\n");
  printf("Tolte le %d forchette dalla tavola\n", n_filosofi);
  printf("Tutti i %d filosofi se ne sono andati\n", n_filosofi);

  return 0;   
}