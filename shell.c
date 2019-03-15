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
#include <sys/stat.h>


#define true 1
#define colorRed "\x1b[31m" //Código de escape ANSI para a cor vermelha
#define colorBlue "\x1b[34m" // Código de escape ANSI para a cor azul
#define colorReset   "\x1b[0m" // Código de escape ANSI para voltar a exibição para a cor padrão do stdout

/*
    casos com erro: "ls" --> não é possível acessar ''$'\350''+'$'\307'')'$'\201\177': Arquivo ou diretório não encontrado
                    "ls -l -a" --> sem retorno. ls -la funcionaria, no entanto
                    "ifconfig" --> erro obtendo informações da interface: %s: dispositivo não encontrado
                    "cd Área\ de\ Trabalho/" --> se torna irresponsivo (possível limitação de chdir com utf-8)
                    "who" --> não exibe nada para qualquer caso

    comandos implementados (ou quase lá):
        pwd (O.K.)
        nano (O.K.)
        quit (O.K.)
        cd (problema com nomes utf-8)
        ls (problema quando chamado sem argumento)
        ping (precisa de Ctrl+C para terminar sua execução, o que também termina o miniShell)
    TO-DO:
        mkdir
        chmod

*/


    //Protótipos - Function signatures
char *getInput();
char **parser(char *input);
void typePrompt();
int run(char ** parsed);
int stringCompare(int str1Length, char * str1, char *str2);
int changeDir(char ** parsed);
void initialize();
int simpleCommand(char ** parsed);
char * getCurrentDirNameOnly();
void stringConcatenate (char *dest, char *src);

    //Variáveis globais
char* acceptableCommands[] = {"ls", "quit", "tryhard"}; // TODO: deletar
char username[32]; // guardará o nome do usuário, com no máximo 32 caracteres, como estipulado pela biblioteca GNU C (glibc).
char hostname[64]; // guardará o nome do host, com no máximo 64 caracteres, como estipulado pela biblioteca GNU C (getconf HOST_NAME_MAX)
int acceptableCommandsNo = 2; // TODO: deletar
int parsedItemsNo; // quantidade de palavras tokenizadas na string atual
char cwd[BUFSIZ]; /* Inicializa string para pasta atual, respeitando o limite máximo de caracteres que o stdin pode transferir */
char * currentDirName;// Deverá ser inicializada em initialize() e então só modificada em changeDir()

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

int run(char ** parsed) // EM TESTE
{
    int sizeFirstWord = strlen(parsed[0]);

    if(stringCompare(sizeFirstWord, parsed[0], "ls"))
        return simpleCommand(parsed);
    else if(stringCompare(sizeFirstWord, parsed[0], "ifconfig"))
        return simpleCommand(parsed);
    else if(stringCompare(sizeFirstWord, parsed[0], "cd"))
        return changeDir(parsed);
    else if(stringCompare(sizeFirstWord, parsed[0], "quit"))
        return -1;
    else if(stringCompare(sizeFirstWord, parsed[0], "pwd"))
    {
        printf("%s\n", cwd);
        return 1;
    }
    else if(stringCompare(sizeFirstWord, parsed[0], "clear"))
    {
        /* O printf abaixo executa um código de escape ANSI para limpar a tela e cursor*/
        printf("\e[2J\e[H");
        return 1;
    }
    else if(stringCompare(sizeFirstWord, parsed[0], "ping") || stringCompare(sizeFirstWord, parsed[0], "nano") || stringCompare(sizeFirstWord, parsed[0], "echo") || stringCompare(sizeFirstWord, parsed[0], "python3"))
        return simpleCommand(parsed);

    //else if(strcmp(parsed[0], ""))
    //else if(strcmp(parsed[0], ""))

    printf("Comando não encontrado.\n");
    return 2;

    //execvp(parsed[0], parsed);
    //printf("%s", acceptableCommands[2]);
}

void typePrompt()
{
    printf(colorBlue "%s@%s:~/" colorRed "%s" colorBlue "$ " colorReset, username, hostname, currentDirName);
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

int changeDir(char ** parsed)
{
    if(parsedItemsNo == 1 || !strcmp(parsed[1],"~") || !strcmp(parsed[1],"..")) // Comandos padrão para retorno para home
    {
        /* procura o ambiente do processo chamante pela variável HOME para que o chdir possa mudar o ambiente da execução para pasta HOMEs */
        printf("Exibição de teste: chdir\n");
        chdir(getenv("HOME"));
        //char * aux = "";
        currentDirName = "";
    }
    else
    {
        printf("parsedItemsNo: %d\n", parsedItemsNo);
        for(int i = 0; i<parsedItemsNo; i++)
            printf("parsed[%d] = %s\n", i, parsed[i]);

        printf("Iniciando concatenação\n\n");

        for(int i=2; i<parsedItemsNo; i++)
        {
            //strcat(parsed[1], " ");
            printf("parsed[%d] = %s\n", i, parsed[i]);
            printf("strlen(parsed[%d]): %zu\n", i, strlen(parsed[i]));
            //strcat(parsed[1], " ");
            strcat(parsed[1], parsed[i]);
            printf("Novo parsed[1]: %s, quando i=%d\n", parsed[1], i);
        }

        printf("%s\n", parsed[1]);

        if(chdir(parsed[1]))
        {
            // Se entrou aqui, houve algum erro na execução de chdir()
            printf("miniShell: cd: %s: Arquivo ou diretório não encontrado\n", parsed[1]);
        }
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

    uid_t uid = geteuid(); // Adquire a ID efetiva do usuário que chamou o shell
    struct passwd *pw = getpwuid(uid); // Procura o UID no banco de dado de senhas e retorna um ponteiro para uma struct passwd
    strcpy(username, pw->pw_name); // Da struct passwd, salva-se o nome do usuário
    gethostname(hostname, 64);
    //printf("\n\n\n%s\n\n\n", pw->pw_name);
    //username = pw->pw_name;
}

int simpleCommand(char ** parsed)
{
    pid_t testPID; // valor para teste pai/filho

    printf("Entrou aqui? 2  %s \n", parsed[1]);
    testPID = fork();
    int * status;
    if ( testPID == 0 )
        execvp(parsed[0], parsed);
    else if (testPID < 0)
        printf("Erro ao produzir fork.");
        return -1;
    }
    else
        waitpid(-1, status, 0);
    return 1; // sucesso
}

    /* Processa a variável cwd para posterior exibição correta em typePrompt()
        exemplo: "/home/vinicius/Downloads" --> "/Downloads"                 */
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

void stringConcatenate (char *dest, char *src)
{
  strcpy (dest + strlen (dest), src);
}
