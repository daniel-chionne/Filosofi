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

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void sparecchia();
void SigIntHandler(int iSignalCode);
void mangia(int i, sem_t *forchetta[]);


int n_filosofi;
pid_t filosofi[256];
sem_t *forchetta[256];
char nome[20][10];

struct sigaction sa;
struct sigaction sa2;
struct timespec tempo;

int fd[2];
int cont_stallo;
int cont_starv;

// Inizializzazione dei flags
int f_stallo;
int f_starv;
int f_sol;


void SigIntHandler(int iSignalCode) {
  sparecchia(); //rimuove le forchette (semafori)
}

void SigAlarmHandler(int iSignalCode){
    printf(ANSI_COLOR_RED "STARVATION RILEVATA DA FILOSOFO CON PID %d\n" ANSI_COLOR_RESET, getpid()); 
    //signal(SIGINT, SigIntHandler);
    for (int i = 0; i < n_filosofi; i++){
      kill(filosofi[i], SIGINT); //uccido i processi
    }
}

void sparecchia(){
  //chiusura dei file descriptor di lettura e scrittura
  close(fd[0]);
  close(fd[1]);
  //chiusura dei semafori
  for (int i = 0; i < n_filosofi; i++){
    sem_close(forchetta[i]);
    sem_unlink(nome[i]);
  }
  return;
}

//FUNZIONE COMPORTAMENTO FILOSOFO
void mangia(int i /*identificatore del filosofo*/, sem_t *forchetta[]) {

  //valori di base assegnati alle forchette
  int destra = i;
  int sinistra = (i+1)%n_filosofi;

  //SOLUZIONE ALLO STALLO
  if (f_sol){
    if (i == n_filosofi - 1){
      // l'ultimo filosofo inverte l’ordine di presa delle forchette
      destra = (i+1)%n_filosofi;
      sinistra = i;
    }
  }

  while (1) {
    tempo.tv_sec = 0;   // secondi
    tempo.tv_nsec = 100000000; // nanosecondi 

    printf("Filosofo %d: ATTENDO la forchetta %d\n", i, destra);
    if (f_starv){
        alarm(8); //viene attivato un tempo di 8 secondi
    }
    sem_wait(forchetta[destra]);
    alarm(0); //se il processo non riesce a bloccare l'alarm a causa del semaforo scatta starvation 
    nanosleep(&tempo, NULL);
    printf("Filosofo %d: PRENDO la forchetta %d\n", i, destra);

    nanosleep(&tempo, NULL); //favorisce lo stallo

    printf("Filosofo %d: ATTENDO la forchetta %d\n", i, sinistra);

    //porzione che rileva lo stallo in caso in cui il flag stallo è attivo
    if (f_stallo){
        read(fd[0], &cont_stallo, sizeof(int));
        cont_stallo++; //fino a che non prendo la forchetta destra segnalo il rischio di stallo
        write(fd[1], &cont_stallo, sizeof(int));
        if(cont_stallo == n_filosofi){
            printf(ANSI_COLOR_RED "STALLO RILEVATO\n" ANSI_COLOR_RESET);
            for (int i = 0; i < n_filosofi; i++){
              kill(filosofi[i], SIGINT); //uccido i processi
            }
        }
    }

    if (f_starv){
        alarm(8); //viene attivato un tempo di 8 secondi
    }
    sem_wait(forchetta[sinistra]);
    alarm(0); //se il processo non riesce a bloccare l'alarm a causa del semaforo scatta starvation
    nanosleep(&tempo, NULL);
    printf("Filosofo %d: PRENDO la forchetta %d\n", i, sinistra);

    //porzione che rileva lo stallo in caso in cui il flag stallo è attivo
    //prevenzione dello stallo -> se riesco a prendere la forchetta sinistra decremento il contatore 
    if (f_stallo){
        read(fd[0], &cont_stallo, sizeof(int));
        cont_stallo--; //se ho preso entrambe le forchette il rischio di stallo non si verifica 
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


int main(int argc, char *argv[]) {

  // segnali
  memset(&sa, '\0', sizeof(struct sigaction)); //mettiamo 0 tutti i campi di sigaction (la struttura) per evitare che ci siano dati di memoria scritti
  memset(&sa2, '\0', sizeof(struct sigaction));
  sa.sa_handler = SigIntHandler;
  sa2.sa_handler = SigAlarmHandler;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGALRM, &sa2, NULL);

  n_filosofi = atoi(argv[1]);
  
  if (n_filosofi < 2) {
    printf("Pochi filosofi a cena. Ce ne devono essere almeno 2\n");
    return EXIT_FAILURE;
  } else {
    printf("Numero di filosofi a cena -> %s\n\n\n", argv[1]);
  }

  if (argv[2] != NULL) f_stallo = atoi(argv[2]);
  if (argv[3] != NULL) f_sol = atoi(argv[3]);
  if (argv[4] != NULL) f_starv = atoi(argv[4]);

  if (f_stallo) printf("Flag stallo attivato\n");
  if (f_sol) printf("Flag soluzione stallo attivato\n");
  if (f_starv) printf("Flag starvation attivato\n");
  
  printf("\n\n");

  //creazione delle forchette
  for (int i = 0; i < n_filosofi; i++) {
    
    sprintf(nome[i], "%d", i);
    
    if ((forchetta[i] = sem_open(nome[i], O_CREAT, S_IRWXU, 1)) == SEM_FAILED) {
      printf("Errore in sem_open, errno = %d\n", errno);
      exit(EXIT_FAILURE);
    }
  }
  
  //filosofi[n_filosofi]; //gruppo di filosofi a cena
  
  //CREAZIONE PIPE PER LA RILEVAZIONE DI STALLO
  if (f_stallo){
    cont_stallo = 0; //contatore utile per i processi per la segnalazione di eventuale stallo
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
  

  printf(ANSI_COLOR_RESET"\n\nCapo del ristorante: Fine della cena. PID %d\n", getppid());
  for (int i = 0; i < n_filosofi; i++){
    printf("Filosofo %d: mi alzo dalla tavola. PID %d\n", i, filosofi[i]);
  }
  printf("Tolte le %d forchette dalla tavola\n", n_filosofi);
  printf("Tutti i %d filosofi se ne sono andati\n", n_filosofi);

  exit(0);
}