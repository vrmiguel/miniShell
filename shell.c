/*
    miniShell (https://github.com/vrmiguel/miniShell)
    By Vinícius R. Miguel, Lucas Saavedra Vaz and Gustavo Bárbaro de Oliveira
    Prof. Bruno Kimura, Ph.D.
    Federal University of São Paulo, Mar. 2019
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define true 1
#define BUFSIZE 1024

//Protótipos
char *getInput();
char **parser(char *input);
void type_prompt();

int main(int argv, char **argc)
{
    //char* input;
    //char** parsed;

    while (true)
    {
        type_prompt();
        char *input = getInput(); // Adquira input
        char cwd[BUFSIZE];        // Inicializa string para pasta atual
        getcwd(cwd, sizeof(cwd)); //Grava o nome da pasta atual
        printf("%s\n", input);
        char **parsed = parser(input);

        free(input); //Para ser executada por último
        free(parsed);
    }
    return 0;
}

char *getInput()
{
    char *input = (char *)malloc(sizeof(char)); //Aloca espaço para primeira letra
    char c = getchar();
    input[0] = c;
    int i = 1;
    while (((c = getchar()) != '\n') && (c != EOF) && (i < BUFSIZE - 2))
    {
        input = realloc(input, (i+1)*sizeof(char)); // Aloca espaço para mais uma letra.
        input[i] = c;                         //Insere letra no vetor
        i++;
    }
    input[i] = '\0';                          //Insere terminador de string.
    printf("From getInput: \"%s\"\n", input); // exibição de teste
    return input;
}

char **parser(char *input)
{
    char **parsed = malloc(sizeof(char *)); //Aloca espaço para primeira palavra
    char *token = strtok(input, " ");
    int i = 0;
    parsed[0] = token;
    printf("From Parser: %s\n", parsed[0]);
    for (;;)
    {
        i++;
        token = strtok(NULL, " ");
        if (token == NULL)
            break;
        parsed = realloc(parsed, (i + 1) * sizeof(char *));
        parsed[i] = token;
        //*(parsed + i) = token;
        printf("From Parser: %s\n", parsed[i]); // print de teste
    }
    return parsed;
}

void type_prompt()
{
    printf("~$ ");
}