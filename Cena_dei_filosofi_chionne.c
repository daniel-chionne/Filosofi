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

void mangia(sem_t *forchetta[], int i /*id filosofo*/, int n_filosofi) {
  while (1) {

    printf("Filosofo %d: sto per prendere la forchetta destra %d\n", i, i);
    sem_wait(forchetta[i]);
    printf("Filosofo %d: sto per prendere la forchetta sinistra %d\n", i,
           (i + 1) % n_filosofi);
    sem_wait(forchetta[(i + 1) % n_filosofi]);

    struct timespec tempo;
    tempo.tv_sec = 8;     // secondi
    tempo.tv_nsec = 8000; // nanosecondi (0.5 secondi)
    printf("Sto mangiando...\n");
    int risultato = nanosleep(&tempo, NULL);
    if (risultato == -1) {
      perror("nanosleep");
      exit(EXIT_FAILURE);
    }

    sem_post(forchetta[i]);
    printf("Filosofo %d: ho lasciato la forchetta destra %d\n", i, i);
    sem_post(forchetta[(i + 1) % n_filosofi]);
    printf("Filosofo %d: ho lasciato la forchetta sinistra %d\n", i, (i + 1) % n_filosofi);
  }

  return;
}

int main(int argc, char *argv[]) {

  // segnali
  struct sigaction sa;
  memset(&sa, '\0', sizeof(struct sigaction));
  sa.sa_handler = SigIntHandler;
  sigaction(SIGINT, &sa, NULL);

  if (argc <= 2) { // perchè il primo è il nome dell'eseguibile e il secondo è
                   // il primo argomento
    printf("Numero di filosofi -> %s\n", argv[1]);
  }

  int n_filosofi = atoi(argv[1]);
  // printf("%d\n", n_filosofi);

  if (n_filosofi < 3) {
    printf("pochi filosofi a cena\n");
  }

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

  // creazione semafori per le forchette
  // creo un array di forchette (semafori)
  sem_t *forchetta[n_filosofi];
  char nome[n_filosofi][10];
  for (int i = 0; i < n_filosofi; i++) {

    sprintf(nome[i], "%d", i);

    if ((forchetta[i] = sem_open(nome[i], O_CREAT, S_IRWXU, 1)) == SEM_FAILED) {
      printf("Errore in sem_open, errno = %d\n", errno);
      exit(EXIT_FAILURE);
    }

    
  }

  for (int i = 0; i < n_filosofi; i++) {
    pid_t pid = fork();

    if (pid == -1) {
      perror("Errore in fork\n");
      exit(EXIT_FAILURE);

    } else if (pid == 0) {
      // child
      printf("Sono il filosofo numero: %d\n", i);
      mangia(forchetta, i, n_filosofi);
      exit(0);

    } else {

      // parent
      waitpid(pid, NULL, 0);

      for (i = 0; i < n_filosofi; i++) {
        sem_close(forchetta[i]);
        sem_unlink(nome[i]);
      }
    }
  }

  return 0;
}