/* Crie um programa que contenha uma matriz de float contendo 3 linhas e 3 colunas.
Imprima o endereço de cada posiçao dessa matriz.  */

#include <stdio.h>
#include <stdlib.h>

int main () {

	float *p, mat[3][3];
    

	for (int i = 0; i<3; i++){
        for (int j = 0; j<3; j++){
            p = &mat[i][j];
            printf("Endereco da posicao [%d][%d] : %p\n", i+1, j+1, p);
        }
    }

	
	
}