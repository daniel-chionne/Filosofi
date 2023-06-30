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


void SigIntHandler(int iSignalCode) {
  printf("Handler: ricevuto signal %d. %s\n", iSignalCode,
         strsignal(iSignalCode));
  return;
}

/*void prendi_forchetta(int i){
    printf("Filosofo %d: ho preso la forchetta %d\n", i, i);
    sem_wait(&forchetta[i]);
    printf("Filosofo %d: ho preso la forchetta %d\n", i, (i + 1) % n_filosofi);
    sem_wait(&forchetta[(i + 1) % n_filosofi]);
}

void lascia_forchetta(int i){
    printf("Filosofo %d: ho lasciato la forchetta %d\n", i, i);
    sem_post(&forchetta[i]);
    printf("Filosofo %d: ho lasciato la forchetta %d\n", i, (i + 1) % n_filosofi);
    sem_post(&forchetta[(i + 1) % n_filosofi]);
}*/


void filosofo(int i /*id filosofo*/, sem_t *forchetta_dx, sem_t *forchetta_sx, int n_filosofi) {
  while (1) {

    struct timespec tempo;
    tempo.tv_sec = 8;     // secondi
    tempo.tv_nsec = 0; // nanosecondi 

    printf("Filosofo %d, prendo la forchetta %d\n", i, i);
    sem_wait(forchetta_dx);
    printf("Filosofo %d, prendo la forchetta %d\n", i, (i+1)%n_filosofi);
    sem_wait(forchetta_sx);


    printf("Filosofo %d: mangio...\n", i);

    if (nanosleep(&tempo, NULL) == -1) {
      perror("nanosleep");
      exit(EXIT_FAILURE);
    }

    sem_post(forchetta_dx);
    sem_post(forchetta_sx);

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

  int n_filosofi = atoi(argv[1]);

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


  sem_t forchetta[n_filosofi];
  pid_t filosofi[n_filosofi];

  //creazione delle forchette
  for (int i = 0; i < n_filosofi; i++) {
    if ((sem_init(&forchetta[i], 1 /* 0 se condiviso tra thread, 1 tra processi*/, 1 /*sem start value*/)) == -1) {
      printf("Errore in sem_init, errno = %d\n", errno);
      exit(1);
    }
  }

  //creazione dei filosofi
  for (int i = 0; i < n_filosofi; i++) {
    filosofi[i] = fork();

    if (filosofi[i] == -1) {
      perror("Errore in fork\n");
      exit(EXIT_FAILURE);

    } else if (filosofi[i] == 0) {
      // child
      printf("Sono il filosofo numero: %d\n", i);
      int dx = i;
      int sx = (i+1)%n_filosofi;

      sleep(i);
      filosofo(i, &forchetta[dx], &forchetta[sx], n_filosofi);
      exit(0);

    } else {
      
    }
  }

    for (int i = 0; i < n_filosofi; i++){
        int wstatus;
        waitpid(filosofi[i], &wstatus, 0);
    }

    for (int i = 0; i < n_filosofi; i++){
        sem_destroy(&forchetta[i]);
    }
    return 0;   
}