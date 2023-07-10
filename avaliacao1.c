// Rodrigo Cesar Mendes dos Santos
// Turma : M2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (){
    
    char palavra[100];
    int tamanhop, i;

    printf("Digite a string: \n");
    gets(palavra);
    tamanhop = strlen(palavra);
    
    for (i=0; i<tamanhop; i++){
        if(palavra[i] == 'a' || palavra[i] == 'A' || palavra[i] == 'b' || palavra[i] == 'B' ){
            printf("\n%c", palavra[i]);
        }
    }

    
}