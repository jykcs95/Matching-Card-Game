#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "game.h"

void swap(char *a, char *b){
    char temp = *a;
    *a = *b;
    *b = temp;
}

void printArray(char arr[],int n, char* output){
    int i;
    for(i = 0; i < n; i++){
        if(i%6==0 && i != 0)
            strncat(output,"\n",1);
	strncat(output,&arr[i],1); 
    }
}

void randomize(char arr[],int n){
    srand (time(NULL));
    int i;
    for(i = n-1; i > 0; i--)
    {
        int j = rand()% (i+1);
        swap(&arr[i],&arr[j]);
    }
}

int check(char x){
    int value = -1;
    if (x == 'a')
        value = 0;
    else if (x == 'b')
        value = 1;
    else if (x == 'c')
        value = 2;
    else if (x == 'd')
        value = 3;
    else if (x == 'e')
        value = 4;
    else if (x == 'f')
        value = 5;
    else if (x == 'g')
        value = 6;
    else if (x == 'h')
        value = 7;
    else if (x == 'i')
        value = 8;
    else if (x == 'j')
        value = 9;
    else if (x == 'k')
        value = 10;
    else if (x == 'l')
        value = 11;
    else if (x == 'm')
        value = 12;
    else if (x == 'n')
        value = 13;
    else if (x == 'o')
        value = 14;
    else if (x == 'p')
        value = 15;
    else if (x == 'q')
        value = 16;
    else if (x == 'r')
        value = 17;

return value;
    
}
void game(char* output_deck, char* output_symbol){
    char cards[] = {'=','&','?','#','!','}','*','&','=','?','@','@','}','*','%','!','#','%'};
    char arr[] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r'};
    strcpy(output_deck,arr);
 
    int n = sizeof(cards)/sizeof(cards[0]);
    randomize(cards,n);    
    strcpy(output_symbol,cards);   
}

//This function will take in the user enterd input and return the associated symbol.
char symbolConversion(char input, char symbolArr[]){
	char value;
	value = symbolArr[check(input)];
	return value;
}


