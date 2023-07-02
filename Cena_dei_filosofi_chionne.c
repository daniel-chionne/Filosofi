#include <asm-generic/errno.h>
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

void sparecchia();
void SigIntHandler(int iSignalCode);
void mangia(int i, sem_t *forchetta[]);


int n_filosofi;
sem_t *forchetta[10];
char nome[20][10];

struct sigaction sa;
struct timespec tempo;
struct timespec ts; //uilizzata per rilevare starvation

int fd[2];
int cont_stallo;

// Inizializzazione dei flags
int f_stallo;
int f_starv;
int f_sol;


void SigIntHandler(int iSignalCode) {
  sparecchia(); //rimuove le forchette (semafori)
  return;
}

void sparecchia(){
  close(fd[0]);
  close(fd[1]);
  for (int i = 0; i < n_filosofi; i++){
    sem_close(forchetta[i]);
    sem_unlink(nome[i]);
  }
  return;
}

//FUNZIONE COMPORTAMENTO FILOSOFO
void mangia(int i /*identificatore del filosofo*/, sem_t *forchetta[]) {

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

  int s1;
  int s2;

  while (1) {
    tempo.tv_sec = 2;   // secondi
    tempo.tv_nsec = 8; // nanosecondi 

    //Nel caso in cui ho il flag starvation attivato e soluzione attivata
    if (f_starv != 0 && f_sol != 0){

      //conta 8 secondi dal tempo corrente
      clock_gettime(CLOCK_REALTIME, &ts);
      ts.tv_sec += 8;

      printf("Filosofo %d: ATTENDO la forchetta %d\n", i, destra);
      s1 = sem_timedwait(forchetta[destra], &ts);
      if (s1 == -1){
        //se errore vuol dire che il tempo è scaduto in quanto il filosofo ha aspettato troppo
        if (errno == ETIMEDOUT) printf("RILEVATA STARVATION da filosofo %d. Non riesco a prendere la forchetta %d\n", i, destra);
        kill(0, SIGKILL); //uccido i processi
      }
      
      printf("Filosofo %d: PRENDO la forchetta %d\n", i, destra);
      printf("Filosofo %d: ATTENDO la forchetta %d\n", i, sinistra);
      s2 = sem_timedwait(forchetta[sinistra], &ts);
      if (s2 == -1){
        //se errore vuol dire che il tempo è scaduto in quanto il filosofo ha aspettato troppo
        if (errno == ETIMEDOUT) printf("RILEVATA STARVATION da filosofo %d. Non riesco a prendere la forchetta %d\n", i, sinistra);
        kill(0, SIGKILL); //uccido i processi
      }
    
      printf("Filosofo %d: PRENDO la forchetta %d\n", i, sinistra);

      printf("Filosofo %d: mangio...\n", i);
      nanosleep(&tempo, NULL);

      sem_post(forchetta[destra]);
      printf("Filosofo %d: RILASCIO la forchetta %d\n", i, destra);
      sem_post(forchetta[sinistra]);
      printf("Filosofo %d: RILASCIO la forchetta %d\n", i, sinistra);
         
    } else {

      printf("Filosofo %d: ATTENDO la forchetta %d\n", i, destra);
      sem_wait(forchetta[destra]);
      printf("Filosofo %d: PRENDO la forchetta %d\n", i, destra);
      //fflush(stdout);

      if (f_stallo != 0) sleep(1); //favorisce lo stallo

      printf("Filosofo %d: ATTENDO la forchetta %d\n", i, sinistra);

      //porzione che rileva lo stallo in caso in cui il flag stallo è attivo
      if (f_stallo != 0){
        read(fd[0], &cont_stallo, sizeof(int));
        cont_stallo++;
        write(fd[1], &cont_stallo, sizeof(int));
        if(cont_stallo == n_filosofi){
          printf("STALLO RILEVATO\n");
          kill(0, SIGKILL); //uccido i processi
        }
      }
      
      sem_wait(forchetta[sinistra]);
      printf("Filosofo %d: PRENDO la forchetta %d\n", i, sinistra);

      //porzione che rileva lo stallo in caso in cui il flag stallo è attivo
      //prevenzione dello stallo -> se riesco a prendere la forchetta sinistra decremento il contatore 
      if (f_stallo != 0){
        read(fd[0], &cont_stallo, sizeof(int));
        cont_stallo--;
        write(fd[1], &cont_stallo, sizeof(int));
      }
      
      printf("Filosofo %d: mangio...\n", i);
      nanosleep(&tempo, NULL);

      sem_post(forchetta[destra]);
      printf("Filosofo %d: RILASCIO la forchetta %d\n", i, destra);
      sem_post(forchetta[sinistra]);
      printf("Filosofo %d: RILASCIO la forchetta %d\n", i, sinistra);    
    }
  }
}


int main(int argc, char *argv[]) {

  // segnali
  memset(&sa, '\0', sizeof(struct sigaction));
  sa.sa_handler = SigIntHandler;
  sigaction(SIGINT, &sa, NULL);

  n_filosofi = atoi(argv[1]);
  
  if (n_filosofi < 2) {
    printf("Pochi filosofi a cena. Ce ne devono essere almeno 2\n");
    return EXIT_FAILURE;
  } else {
    printf("Numero di filosofi a cena -> %s\n\n\n", argv[1]);
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
  
  printf("\n\n");

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
      mangia(i, forchetta);
      exit(0);
    }
  }
  
  //il padre sta in attesa dei filosofi
  for (int i = 0; i < n_filosofi; i++){
    waitpid(filosofi[i], NULL, 0);
  }
    
  printf("\n\nCapo: Fine della cena\n");
  printf("Tolte le %d forchette dalla tavola\n", n_filosofi);
  printf("Tutti i %d filosofi se ne sono andati\n", n_filosofi);

  return 0;   
}