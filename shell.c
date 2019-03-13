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
#include <errno.h>


#define true 1

    //Protótipos - Function signatures
char *getInput();
char **parser(char *input);
void typePrompt();
void printWelcome();
int run(char ** parsed);
int strcmpr(char * str1, char *str2);


    //Variáveis globais
//char* acceptableCommands[] = {"ifconfig", "ls", "quit"};
char* acceptableCommands[] = {"ls", "quit"};
int acceptableCommandsNo = 2;
int parsedItemsNo; // quantidade de palavras tokenizadas na string atual

int main(int argv, char **argc)
{
    //char* input;
    //char** parsed;
    printWelcome();
    char cwd[BUFSIZ]; /* Inicializa string para pasta atual, respeitando o limite máximo de caracteres que o stdin pode transferir */
    while (true)
    {
        typePrompt(); // Exibe "~$ "
        int ret; // Variável para teste de retorno de execução | execution r
        char *input = getInput(); // Adquira input
        getcwd(cwd, sizeof(cwd)); //Grava o nome da pasta atual
        char **parsed = parser(input);
        ret = run(parsed);
        free(input); //Libera memória alocada para input e parsed
        free(parsed);
    }
    return 0;
}

char *getInput()
{
    /* Lê e retorna caracteres do stdin. Por motivos de economia de memória, o programa salva a
    string do usuário de maneira dinâmicamente crescente. */
    char *input = (char *) malloc(sizeof(char)); //Aloca espaço para primeira letra
    char c = getchar(); // Recebe primeira letra..
    input[0] = c;       //..e inicializa input com ela.

    int i = 1;

    while (((c = getchar()) != '\n') && (c != EOF) && (i < BUFSIZ - 2))
    {
        input = realloc(input, (i+1)*sizeof(char)); // Aloca espaço para mais uma letra
        input[i] = c;    //Insere letra no vetor
        i++;
    }
    input[i] = '\0';    //Insere terminador de string.
    return input;
}

/* Divide a string dada pela variável input  */
char **parser(char *input)
{
    char **parsed = malloc(sizeof(char *)); //Aloca espaço para primeira palavra
    char *token = strtok(input, " "); // Cria primeiro token e mantém o ponteiro input em estado interno
    int i = 0;
    parsed[0] = token; // Inicializa parsed com o primeiro token

    // printf("Do Parser: %s\n", parsed[0]); // teste

    for (;;)
    {
        token = strtok(NULL, " "); //Continua criando tokens do ponteiro em estado interno
        if (token == NULL) //Caso encontrada o fim da string original, feche o loop
            break;
        i++;
        parsed = realloc(parsed, (i + 1) * sizeof(char *)); // Aloca espaço para mais uma palavra
        parsed[i] = token;
        //printf("Do Parser: %s\n", parsed[i]); // print de teste
    }
    printf("ParsedListSize = %d\n", i);
    parsedItemsNo = i+1; // Registra quantas palavras foram tokenizadas
    return parsed;
}

int run(char ** parsed) // EM TESTE, NÃO FUNCIONA
{
    pid_t testPID; // valor para teste pai/filho
    int i, aux, foundIndex = -1;
    if(parsedItemsNo<=acceptableCommandsNo)
      aux = acceptableCommandsNo;
    else
      aux = parsedItemsNo;

    for(i=0; i<aux-1; i++)
    {
        if( !strcmpr(parsed[i], acceptableCommands[i]) )
            printf("Deu certo com %d\n", i);
    }

    //execvp(parsed[0], parsed);
    //printf("%s", acceptableCommands[2]);
    return 1;
}

void typePrompt()
{
    printf("~$ ");
}

void printWelcome()
{
    printf("\n   -------- miniShell --------\n\nVinícius R. Miguel, Lucas S. Vaz & Gustavo B. de Oliveira\ngithub.com/vrmiguel/miniShell -- Unifesp -- Março de 2019\n\n");
}

int strcmpr(char* s1, char* s2)
{
    while(*s1 && (*s1==*s2))
        s1++,s2++;
    return *(unsigned char*)s1-*(unsigned char*)s2;
}
