#include <stdio.h>
#include <stdlib.h>


int main (){

int n;
printf("\nDigite n: ");
scanf("%d", &n);

int nums[n*2];

for (int i=0; i<n*2; i++){
    printf("\nDigite o elemento[%d] : ", i+1);
    scanf("%d", &nums[i]);
};
for (int j = 0; j<n; j++){
    printf("%d,%d,", nums[j], nums[j+3]);
};

} /*
int* shuffle(int* nums, int numsSize, int n, int* returnSize){
printf("Digite n : \n");
scanf("%d", &n);

nums[n*2];

for(int i = 0; i printf("Elemento[%d]", i);
scanf("%d", &nums[i]);
}

for (int j = 0; j for(int k = 1; k printf("%d, %d,", nums[j], nums[k]);
}
}
} */
