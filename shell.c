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
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define true 1
#define colorRed "\x1b[31m"
#define colorBlue "\x1b[34m"

/* 
    casos com erro: "ls" --> não é possível acessar ''$'\350''+'$'\307'')'$'\201\177': Arquivo ou diretório não encontrado
                    "ifconfig" --> erro obtendo informações da interface: %s: dispositivo não encontrado
                    "cd Área\ de\ Trabalho/" --> se torna irresponsivo (possível limitação de chdir com utf-8)

    comandos implementados (ou quase lá):
        cd
        ls
        pwd
        quit
*/


    //Protótipos - Function signatures
char *getInput();
char **parser(char *input);
void typePrompt();
int run(char ** parsed);
int stringCompare(int str1Length, char * str1, char *str2);
int changeDirectory(char ** parsed);
void initialize();
int simpleCommand(char ** parsed);
char * getCurrentDirNameOnly (); 

    //Variáveis globais
char* acceptableCommands[] = {"ls", "quit", "tryhard"}; // TODO: deletar
char username[32]; // guardará o nome do usuário, com no máximo 32 caracteres, como estipulado pela biblioteca GNU C (glibc).
char hostname[64]; // guardará o nome do host, com no máximo 64 caracteres, como estipulado pela biblioteca GNU C (getconf HOST_NAME_MAX)
int acceptableCommandsNo = 2; // TODO: deletar
int parsedItemsNo; // quantidade de palavras tokenizadas na string atual
char cwd[BUFSIZ]; /* Inicializa string para pasta atual, respeitando o limite máximo de caracteres que o stdin pode transferir */
char * currentDirName;// Deverá ser inicializada em initialize() e então só modificada em changeDirectory()

int main(int argv, char **argc)
{
    initialize();
    while (true)
    {
        typePrompt(); // Exibe prompt padrão de digitação
        int ret; // Variável para teste de retorno de execução | execution r
        char *input = getInput(); // Adquira input
        char **parsed = parser(input);
        ret = run(parsed);
        free(input); //Libera memória alocada para input e parsed
        free(parsed);
        if (ret == -1)
            break;
    }
    printf("Finalizando miniShell\n");
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
    char *token = strtok(input, " "); // Cria primeiro token (separador " ") e mantém o ponteiro input em estado interno
    int i = 0;
    parsed[0] = token; // Inicializa parsed com o primeiro token

    // printf("Do Parser: %s\n", parsed[0]); // teste

    for (;;) // loop infinito de uma maneira chic :)
    {
        token = strtok(NULL, " "); //Continua criando tokens do ponteiro em estado interno
        if (token == NULL) //Caso encontrada o fim da string original, feche o loop
            break;
        i++;
        parsed = realloc(parsed, (i + 1) * sizeof(char *)); // Aloca espaço para mais uma palavra
        parsed[i] = token;
        //printf("Do Parser: %s\n", parsed[i]); // print de teste
    }
    
    parsedItemsNo = i+1; // Registra quantas palavras foram tokenizadas

    //for(int i = 0; i<parsedItemsNo; i++)
    //    printf("t: %s\n", parsed[i]);
    return parsed;
}

int run(char ** parsed) // EM TESTEs
{
    int i, sizeFirstWord = strlen(parsed[0]);

    if(stringCompare(sizeFirstWord, parsed[0], "ls"))
        return simpleCommand(parsed);
    else if(stringCompare(sizeFirstWord, parsed[0], "ifconfig"))
        return simpleCommand(parsed);
    else if(stringCompare(sizeFirstWord, parsed[0], "cd"))
        return changeDirectory(parsed);
    else if(stringCompare(sizeFirstWord, parsed[0], "quit"))
        return -1;
    else if(stringCompare(sizeFirstWord, parsed[0], "pwd"))
        printf("%s\n", cwd);
        return 1;
    
    //else if(strcmp(parsed[0], ""))
    //else if(strcmp(parsed[0], ""))


    //execvp(parsed[0], parsed);
    //printf("%s", acceptableCommands[2]);
    return 1;
}

void typePrompt()
{
    printf("%s@%s:/%s$ ", username, hostname, currentDirName);
}

int stringCompare(int str1Length, char* str1, char* str2) // não usado, possivelmente será excluído
{
    int a = strlen(str1);
    if (a != strlen(str2))
        return 0;
    
    for(int i = 0; i<a; i++)  
        if(str1[i] != str2[i])
            return 0; 
    return 1;
}

int changeDirectory(char ** parsed)
{
    if(parsedItemsNo == 1 || !strcmp(parsed[1],"~") || !strcmp(parsed[1],".."))
    {
        chdir(getenv("HOME"));
        char * aux = "";
        currentDirName = aux;
    }
    else
    {
        chdir(parsed[1]);
        currentDirName = getCurrentDirNameOnly();
    }
    getcwd(cwd, BUFSIZ);//Grava o nome da pasta atual - TODO - desnecessário ?
    return 1;
}

void initialize()
{
    printf("\n   -------- miniShell --------\n\nVinícius R. Miguel, Lucas S. Vaz & Gustavo B. de Oliveira\ngithub.com/vrmiguel/miniShell -- Unifesp -- Março de 2019\n\n");
    getcwd(cwd, BUFSIZ); //Grava o nome da pasta atual
    //printf("CWD: %s\n", cwd);
    currentDirName = getCurrentDirNameOnly(); // Formata a string da pasta atual para exibição em typePrompt()
    //printf("Current Dir: %s\n", currentDirName);
    
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    strcpy(username, pw->pw_name);
    gethostname(hostname, 64);
    //printf("\n\n\n%s\n\n\n", pw->pw_name);
    //username = pw->pw_name;
}

int simpleCommand(char ** parsed)
{
    pid_t testPID; // valor para teste pai/filho
    testPID = fork();
    int * status;

    if ( testPID == 0 )
        execvp(parsed[0], parsed);
    else
        waitpid(-1, status, 0);
    return 1; // sucesso
}

    /* Processa a variável cwd para posterior exibição correta em typePrompt() 
        exemplo: "/home/vinicius/Downloads" --> "/Downloads                 */
char * getCurrentDirNameOnly () 
{
    char stringAux[BUFSIZ];
    getcwd(stringAux, BUFSIZ);

    char *aux;
    char *token = strtok(stringAux, "/"); // Cria primeiro token (separador "/") e mantém o ponteiro stringAux em estado interno.
    for(;;)
    {
        token = strtok(NULL, "/"); // Adiciona mais um token do ponteiro em estado interno (stringAux).
        if(token == NULL)
            break;
        aux = token;
    }
    return aux;
}
