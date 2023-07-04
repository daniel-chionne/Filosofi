#!/bin/bash

file=Cena_filosofi.c

chmod 777 "$file"

if test -f $file 
then
    echo -e "File esistente\n\n"
    echo -e "CENA DEI FILOSOFI\n"

    echo -e "INSERISCI I FLAGS"

    echo -n -e "\e[32mnumero filosofi -> \e[0m"
    read n_filosofi
    echo -n -e "\e[32mrilevazione stallo -> \e[0m"
    read f_stallo
    echo -n -e "\e[32msoluzione stallo -> \e[0m"
    read f_sol
    echo -n -e "\e[32mrilevazione starvation -> \e[0m"
    read f_starv

    gcc -o cena_chionne "$file" -pthread
    ./cena_chionne "$n_filosofi" "$f_stallo" "$f_sol" "$f_starv"
else
    echo "File non esistente"
fi

exit 1
