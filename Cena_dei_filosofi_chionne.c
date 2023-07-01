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

int n_filosofi;
sem_t *forchetta[5];
char nome[5][10];

void togli_forchette(){
    for (int i = 0; i < n_filosofi; i++){
        sem_close(forchetta[i]);
        sem_unlink(nome[i]);
    }
    return;
}
  
void SigIntHandler(int iSignalCode) {
  //printf("Handler: ricevuto signal %d. %s\n.", iSignalCode,strsignal(iSignalCode));
  togli_forchette();
  return;
}

void filosofo(int i, sem_t *forchetta[], int n_filosofi) {
  while (1) {

    struct timespec tempo;
    tempo.tv_sec = 0;     // secondi
    tempo.tv_nsec = 8; // nanosecondi 

    printf("Filosofo %d, ATTENDO la forchetta %d\n", i, i);
    sem_wait(forchetta[i]);
    printf("Filosofo %d, ATTENDO la forchetta %d\n", i, (i+1)%n_filosofi);
    sem_wait(forchetta[(i+1)%n_filosofi]);


    printf("Filosofo %d: mangio...\n", i);

    if (nanosleep(&tempo, NULL) == -1) {
      perror("nanosleep");
      exit(EXIT_FAILURE);
    }

    printf("Filosofo %d, RILASCIO la forchetta %d\n", i, i);
    sem_post(forchetta[i]);
    printf("Filosofo %d, RILASCIO la forchetta %d\n", i, (i+1)%n_filosofi);
    sem_post(forchetta[(i+1)%n_filosofi]);

    printf("Filosofo %d: ho finito di mangiare\n", i);
  }
  
}

int main(int argc, char *argv[]) {

  // segnali
  struct sigaction sa;
  memset(&sa, '\0', sizeof(struct sigaction));
  sa.sa_handler = SigIntHandler;
  sigaction(SIGINT, &sa, NULL);


  if (argc <= 2) { // perchè il primo è il nome dell'eseguibile e il secondo è il primo argomento
    printf("Numero di filosofi -> %s\n", argv[1]);
  }

  n_filosofi = 4 /*atoi(argv[1])*/;

  if (n_filosofi < 2) {
    printf("pochi filosofi a cena\n");
  }

/*
  // Inizializzazione dei flags
  int f_stallo = 0;
  int f_starv = 0;
  int f_sol = 0;

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
*/


  //creazione delle forchette
  for (int i = 0; i < n_filosofi; i++) {
      
    sprintf(nome[i], "%d", i);
    
    if ((forchetta[i] = sem_open(nome[i], O_CREAT, S_IRWXU, 1)) == SEM_FAILED) {
      printf("Errore in sem_open, errno = %d\n", errno);
      exit(EXIT_FAILURE);
    }
  }
  
  pid_t filosofi[n_filosofi]; //gruppo di filosofi a cena

  //creazione dei filosofi
  for (int i = 0; i < n_filosofi; i++) {
    filosofi[i] = fork();

    if (filosofi[i] == -1) {
      perror("Errore in fork\n");
      exit(EXIT_FAILURE);

    } else if (filosofi[i] == 0) {
      // child
      printf("Sono il filosofo numero: %d\n", i);
      filosofo(i, forchetta, n_filosofi);
      exit(0);
    }
  }

    for (int i = 0; i < n_filosofi; i++){
        /*int wstatus;
        waitpid(filosofi[i], &wstatus, 0);*/
        wait(NULL);
    }
    
    togli_forchette(); //metodo di rimozione delle forchette (semafori)
    
    //Messaggi signal (per evitare che venga chiamato esmplicitamente tante volte quanti sono i processi)
    printf("Handler: ricevuto signal di Interrupt\n");
    printf("tolte le %d forchette dalla tavola\n", n_filosofi);

    return 0;   
}