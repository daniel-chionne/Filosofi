#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

sem_t *forchetta;

void mangia(){
    while(1){
        //prendi la forchetta destra
        printf("filosofo %d: ho preso la forchetta destra", getpid());
        //prendi la forchetta destra
        printf("filosofo %d: ho preso la forchetta sinistra", getpid());
    }
}

int main(int argc, char *argv[]) {

  if (argc <= 2) { // perchè il primo è il nome dell'eseguibile e il secondo è
                   // il primo argomento
    printf("Numero di filosofi -> %s\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  int n_filosofi = atoi(argv[1]);
  // printf("%d\n", n_filosofi);

  if (n_filosofi < 3) {
    printf("pochi filosifi a cena\n");
    exit(EXIT_FAILURE);
  }

  // Inizializzazione dei flags
  int f_stallo = 0;
  int f_starv = 0;
  int f_sol = 0;

  if (argv[2] != NULL) f_stallo = atoi(argv[2]);
  if (argv[3] != NULL) f_sol = atoi(argv[3]);
  if (argv[4] != NULL) f_starv = atoi(argv[4]);

  if (f_stallo != 0) printf("Flag stallo attivato\n");
  if (f_sol != 0) printf("Flag soluzione stallo attivato\n");
  if (f_starv != 0) printf("Flag starvation attivato\n");


    //creazione semafori
    for (int i = 1; i <= n_filosofi; i++){
        if((forchetta = sem_open((char)i, O_CREAT, S_IRWXU, 0)) == SEM_FAILED){
        printf("Errore in sem_open, errno = %d\n", errno);
        exit(EXIT_FAILURE);
    }
    }
    for (int i = 1; i <= n_filosofi; i++) {
        pid_t pid = fork();

        if (pid == -1) {
        perror("Errore in fork\n");
        exit(EXIT_FAILURE);

        }else if (pid == 0) {
            // child
            printf("Sono primo filosofo numero: %d\n", getpid());
            
            exit(0);

        } else {

            // parent
            int wstatus;
            wait(&wstatus); // se commentate questa riga il child sarà "zombie"
            if (WIFEXITED(wstatus))
                printf("Il filosofo ha finito di mangiare. Exit status = %d\n", WEXITSTATUS(wstatus));
            else
                printf("Il child NON ha finito di mangiare!!!\n");

            printf("Saluti dal salotto del capo. Capo pid = %d\n", getppid());
            fflush(stdout);
        }
    }

  return 0;
}