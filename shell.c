/*
    miniShell (https://github.com/vrmiguel/miniShell)
    By Vinícius R. Miguel and Gustavo Bárbaro de Oliveira
    Prof. Bruno Kimura, Ph.D.
    Federal University of São Paulo, Mar. 2019
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define true 1
#define false 0
#define colorRed "\x1b[31m" //Código de escape ANSI para a cor vermelha
#define colorBlue "\x1b[34m" // Código de escape ANSI para a cor azul
#define colorReset   "\x1b[0m" // Código de escape ANSI para voltar a exibição para a cor padrão do stdout
#define WRITE_END 1
#define READ_END 0

//Protótipos
char *getInput(void);
char **parser(char *input);
void typePrompt(void);
int run(char ** parsed);
int changeDir(char ** parsed);
void initialize(void);
int simpleCommand(char ** parsed);
int pipedCommand(char ** parsed);
char * getCurrentDirNameOnly(void);
int  findPipe(char ** parsed);
int  findRedirectToFile(char ** parsed);

//Variáveis globais
char username[32]; // guardará o nome do usuário, com no máximo 32 caracteres, como estipulado pela biblioteca GNU C (glibc).
char hostname[64]; // guardará o nome do host, com no máximo 64 caracteres, como estipulado pela biblioteca GNU C (getconf HOST_NAME_MAX)
int parsedItemsNo; // quantidade de palavras tokenizadas na string atual
char cwd[BUFSIZ]; /* Inicializa string para pasta atual, respeitando o limite máximo de caracteres que o stdin pode transferir */
char * currentDirName;// Deverá ser inicializada em initialize() e então só modificada em changeDir()
int pipePositions[10] = {0};
int nPipes = 0;
int writeToPosition = 0;
int readFromPosition = 0;
int bg = false;

int main(int argv, char ** argc)
{
    initialize();
    for (;;)
    {
        if(bg == false)
            typePrompt(); // Exibe prompt padrão de digitação
        int ret; // Variável para teste de retorno de execução | execution r
        char *input = getInput(); // Adquira input
        char ** parsed = parser(input);
        ret = run(parsed);

        free(input); //Libera memória alocada para input e parsed

        for(int i=0; i<parsedItemsNo; i++)
            free(parsed[i]);
        free(parsed);
        if (ret == -1)
            break;
    }
    printf("Closing miniShell\n");
    return 0;
}

int saveToFile(char ** parsed)
{
    int out = open(parsed[writeToPosition+1], O_RDWR|O_CREAT|O_TRUNC, 0600);
    if (out == - 1)
    {
        fprintf(stderr, "Erro em criação de arquivo \"%s\"", parsed[writeToPosition+1] );
        return 0;
    }
    int output = dup(fileno(stdout));
    if (dup2(out, fileno(stdout)) == -1)
    {
        fprintf(stderr, "Problema em redirecionamento de stdout.\n");
        return 0;
    }

    int i;
    if (fork() == 0)
    {
        char ** aux = malloc(parsedItemsNo * sizeof(char *));
        for(i=0; i<writeToPosition; i++)
            aux[i] = parsed[i];
        execvp(aux[0], aux);
        fprintf(stderr, "%s: Comando não encontrado.\n", aux[0]);
        free(aux);
    }
    else
        wait(0);
    fflush(stdout);
    close(out);
    dup2(output, fileno(stdout));
    close(output);
    writeToPosition = 0;
    return 1;
}

char *getInput()
{
    /* Lê e retorna caracteres do stdin. Por motivos de economia de memória, o programa salva a
    string do usuário de maneira dinâmicamente crescente. */
    char *input = malloc(BUFSIZ*sizeof(char)); //Aloca espaço para letras (tamanho máximo de buffer de stdin)
    char c = getchar(); // Recebe primeira letra..
    input[0] = c;       //..e inicializa input com ela.
    int i;

    /* Lê caracteres enquanto o usuário não pressiona Enter ou Ctrl-D e enquanto o comando de entrada não passa do tamanho máximo de buffer de stdin. */
    for(i=1; (i < BUFSIZ - 2) && ((c = getchar()) != '\n') && (c != EOF); ++i)
        input[i] = c;    //Insere letra no vetor

    input[i] = '\0';   //Insere terminador de string.
    return input;
}

/* Divide a string dada pela variável input  */
char **parser(char *input)
{
    char **parsed = malloc(20 * sizeof(char *)); //Aloca espaço para vinte palavras
    char * pos;
    char *token = strtok_r(input, " ", &pos); // Cria primeiro token (separador " ") e ... TODO
    int i = 0;
    parsed[0] = malloc(strlen(token)*sizeof(char));
    strcpy(parsed[0],token); // Inicializa parsed com o primeiro token

    for (;;)
    {
        token = strtok_r(NULL, " ", &pos); // .. TODO
        if (token == NULL) //Caso encontrada o fim da string original, feche o loop
            break;
        i++;
        //parsed = realloc(parsed, (i + 1) * sizeof(char *)); // Aloca espaço para mais uma palavra
        parsed[i] = malloc(strlen(token) + 1);
        strcpy(parsed[i], token);
        //printf("Do Parser: %s\n", parsed[i]); // print de teste
    }
    //parsed = realloc(parsed, (i + 1) * sizeof(char *));
    parsedItemsNo = i+1; // Registra quantas palavras foram tokenizadas
    parsed[i+1] = NULL; // Necessário para ser argumento na família exec()

    return parsed;
}

int findRedirectToFile(char ** parsed)
{
    int i;
    for (i=0;i<parsedItemsNo;i++)
        if(strcmp(parsed[i], ">") == 0)
        {
            writeToPosition = i;
            return 1;
        }
    return 0;
}

int findRedirectFromFile(char ** parsed)
{
    int i;
    for (i=0;i<parsedItemsNo;i++)
        if(strcmp(parsed[i], "<") == 0)
        {
            readFromPosition = i;
            return 1;
        }
    return 0;
}

int readFromFile(char ** parsed)
{
    int i;

    int inp = open(parsed[readFromPosition+1], O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (inp == -1)
    {
        fprintf(stderr, "Erro em leitura de arquivo \"%s\"", parsed[readFromPosition+1]);
        return 0;
    }

    int input = dup(fileno(stdin));
    if (dup2(inp, fileno(stdin)) == -1)
    {
        fprintf(stderr, "Problema em redirecionamento de stdin.\n");
        return 0;
    }

    char ** aux = malloc(readFromPosition * sizeof(char *));
    for(i=0; i<readFromPosition; i++)
        aux[i] = parsed[i];

    //pid_t pid = fork();

    if (fork() == 0)
    {
        execvp(aux[0], aux);
        fprintf(stderr, "%s: Comando não encontrado.\n", aux[0]);
        free(aux);
    }
    //else if (pid < 0)
    //    fprintf(stderr, "Erro em criação de fork. \n");
    else
        wait(0);

    //fflush(stdout);
    close(inp);
    dup2(input, fileno(stdin));
    close(input);
    readFromPosition = 0;
    return 1;
}

int run(char ** parsed)
{
    if (findPipe(parsed))
        return pipedCommand(parsed);

    else if(findRedirectToFile(parsed))
        return saveToFile(parsed);

    else if(findRedirectFromFile(parsed))
        return readFromFile(parsed);

    else
    {  // Caso não  haja nenhum pipe
        if(!strcmp(parsed[0] , "cd"))
            return changeDir(parsed);
        else if(!strcmp(parsed[0], "quit") || !strcmp(parsed[0],  "exit"))
            return  -1;
        else if(!strcmp(parsed[0], "clear"))
        {
            /* O printf abaixo executa um código de escape ANSI para limpar a tela e cursor*/
            printf("\e[2J\e[H");
            return 1;
        }
        else if(!strcmp(parsed[0], "pwd"))
        {
            printf("%s\n", cwd);
            return 1;
        }
        else if(!strcmp(parsed[0], "help"))
        {
            printf("Este shell suporta: \n");
            printf("  -Comandos simples, tais como \'ls\' ou \'ls -li\'\n");
            printf("  -Salvamento de comando simples em arquivo. Ex: \'ls -li > txt\'\n");
            printf("  -Leitura de arquivo para comando simples. Ex: \'sort < txt\'\n");
            printf("  -Comandos encadeados em pipe (no máximo dez pipes)\n");
            printf("  -Execução de comandos simples e encadeados em background.\n");
            return 0;
        }
            else return simpleCommand(parsed);
    }
}

int simpleCommand(char ** parsed)
{
    pid_t testPID; // valor para teste pai/filho
    testPID = fork();

    if(!strcmp(parsed[parsedItemsNo-1], "&"))
    {
        bg = true;
        free(parsed[parsedItemsNo-1]);
        parsed[parsedItemsNo-1] = NULL;
        parsedItemsNo--;
    }

    int status;
    if (testPID == 0)
    {
        execvp(parsed[0], parsed);
        fprintf(stderr, "%s: comando não encontrado\n", parsed[0]);
    }
    else if (testPID < 0)
    {
        printf("Erro ao produzir fork.");
        return 0;
    }
    else
        if(!bg)
            waitpid(-1, &status, WUNTRACED);

    return 1; // sucesso
}

// Obtem comandos auxiliares para execução em pipedCommand()
void getAuxCommand(char ** parsed, char ** aux, int loop)
{
    int i, c;
    if (loop == 0)
    {
        for(c = 0; c < pipePositions[0]; c++) // Copia parsed até chegar no primeiro "|"
            aux[c] = parsed[c];
        //fprintf(stderr, "loop: %d", loop);
        aux[c+1] = NULL; // Argumentos para a família exec devem terminar com NULL
        return;
    }
    else
    {
        c = 0;
        for(i=pipePositions[loop-1]+1;  i<pipePositions[loop];  i++)
        {
            aux[c] = parsed[i];
            c++;
        }
        aux[c+1] = NULL;
    }
}

int pipedCommand(char** parsed){
    int nCommands = nPipes + 1;
    int fileDescriptor[10][2], i;

    if(!strcmp(parsed[parsedItemsNo-1], "&"))
    {
        bg = true;
        free(parsed[parsedItemsNo-1]);
        parsed[parsedItemsNo-1] = NULL;
        parsedItemsNo--;
    }

    char ** aux = NULL;
    for(i=0; i<nCommands; i++)
    {
        if (aux != NULL)
            free(aux);
        char ** aux = malloc(parsedItemsNo * sizeof(char*));
        getAuxCommand(parsed, aux, i);
        if(i!=nCommands-1)
            if(pipe(fileDescriptor[i])<0)
            {
                fprintf(stderr, "Erro na criação de pipe.\n");
                return -2;
            }
        if(fork()==0) // Primeiro processo filho
        {
            if(i!=nCommands-1)
            {
                dup2(fileDescriptor[i][WRITE_END],STDOUT_FILENO);
                close(fileDescriptor[i][READ_END]);
                close(fileDescriptor[i][WRITE_END]);
            }
            if(i!=0)
            {
                dup2(fileDescriptor[i-1][READ_END],STDIN_FILENO);
                close(fileDescriptor[i-1][WRITE_END]);
                close(fileDescriptor[i-1][READ_END]);
            }
            execvp(aux[0], aux);
            fprintf(stderr, "%s: Comando não encontrado.\n", aux[0]);
            return -2;
        }
        else
            if(i!=0)
            {
                close(fileDescriptor[i - 1][READ_END]);
                close(fileDescriptor[i - 1][WRITE_END]);
            }
    }
    if (!bg)
      for(i=0; i<nCommands; i++)
          wait(NULL);
    for(i=0; i<10; i++) // Reseta posições
        pipePositions[i] = 0;
    free(aux);
    nPipes = 0; // Reseta número encontrado de pipes
    return 1;
}

/* Encontra pipes no comando lido, salvando suas posições em pipePositions */
int findPipe(char ** parsed)
{
    int i, j, c=0;
    for(i=0; i<parsedItemsNo; i++) // Varre os comandos lidos procurando por um caracter "|"
        if(strcmp(parsed[i], "|")==0)
            for(j=0; j<11; j++)
                if (pipePositions[c]==0) // Se a posição atual ainda .TODO
                {
                    nPipes++;
                    pipePositions[c] = i;
                    c++;
                    break;
                }
    if(nPipes > 0)
        pipePositions[c] = parsedItemsNo;

    return nPipes > 0; // Retorna verdadeiro se algum "|" foi encontrado, falso caso contrário.
}

void typePrompt()
{
    printf(colorBlue "%s@%s:~/" colorRed "%s" colorBlue "$ " colorReset, username, hostname, currentDirName);
}

    /* muda o diretório do processo pai */
int changeDir(char ** parsed)
{
    if(parsedItemsNo == 1 || !strcmp(parsed[1],"~")) // Comandos padrão para retorno para home
    { // TODO: mudar 'cd ..'
        /* procura o ambiente do processo chamante pela variável HOME para que o chdir possa mudar o ambiente da execução para pasta HOMEs */
        printf("Exibição de teste: chdir\n");
        chdir(getenv("HOME"));
        currentDirName = "";
    }
    else
    {
        printf("parsedItemsNo: %d\n", parsedItemsNo);
        for(int i = 0; i<parsedItemsNo; i++)
            printf("parsed[%d] = %s\n", i, parsed[i]);

        //printf("Iniciando concatenação\n\n");

        for(int i=2; i<parsedItemsNo; i++)
        {
            strcat(parsed[1], " ");
            strcat(parsed[1], parsed[i]);
            //printf("Novo parsed[1]: %s, quando i=%d\n", parsed[1], i);
        }

        if(chdir(parsed[1]))
        {
            // Se entrou aqui, houve algum erro na execução de chdir()
            printf("miniShell: cd: %s: Arquivo ou diretório não encontrado\n", parsed[1]);
        }
        else
            currentDirName = getCurrentDirNameOnly(); // caso de sucesso
    }
    getcwd(cwd, BUFSIZ);//Grava o nome da pasta atual - TODO - desnecessário ?
    return 1;
}

void initialize()
{
    printf("\n   -------- miniShell --------\n\nVinícius R. Miguel & Gustavo B. de Oliveira\ngithub.com/vrmiguel/miniShell -- Unifesp -- Março de 2019\n");
    printf("Digite \'help\' para obter ajuda.\n\n");
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

/* Processa a variável cwd para posterior exibição correta em typePrompt()
    exemplo: "/home/vinicius/Downloads" --> "/Downloads"                 */
char * getCurrentDirNameOnly ()
{
    char stringAux[BUFSIZ];
    getcwd(stringAux, BUFSIZ); // Armazena diretório atual
    char *aux;
    char *token = strtok(stringAux, "/"); // Cria primeiro token (separador "/") e mantém o ponteiro stringAux em estado interno.
    for (;;) {
        token = strtok(NULL, "/"); // Adiciona mais um token do ponteiro em estado interno (stringAux).
        if (token == NULL)
            break;
        aux = token;
    }
    return aux;
}
