#include <bits/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


int main(int argc, char *argv[]){

    if (!isdigit(*argv[1])){
        perror("Non ho roconosciuto un numero");
        exit(-1);
    }

    //printf("%d\n", atoi(argv[1]));

    if (atoi(argv[1]) < 3){
        perror("Pochi filosofi a cena!!");
        exit(-1);
    }
    if (atoi(argv[2]) != 0){
        printf("rilevazione stallo attivata\n");
    }
    if (atoi(argv[3]) != 0){
        printf("rilevazione soluzione stallo attivata\n");
    }
    if (atoi(argv[4]) != 0){
        printf("rilevazione starvation attivata\n");
    }


    return 0;
}