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

/*
    casos com erro: "ls" --> não é possível acessar ''$'\350''+'$'\307'')'$'\201\177': Arquivo ou diretório não encontrado
                    "ls -l -a" --> sem retorno. ls -la funcionaria, no entanto
                    "ifconfig" --> erro obtendo informações da interface: %s: dispositivo não encontrado
                    "cd Área\ de\ Trabalho/" --> se torna irresponsivo (possível limitação de chdir com utf-8)
                    "who" --> não exibe nada para qualquer caso
    comandos implementados (ou quase lá):
    TO-DO:
        mkdir
        chmod
        checar por built-ins em pipes
        implementar vetor 'commands', feito a partir de parsed, contendo
        a cadeia de comandos a ser executada.
    para liberar:
        parsed, input
*/

        //Protótipos
char *getInput(void);
char **parser(char *input);
void typePrompt(void);
int run(char ** parsed);
int stringCompare(int str1Length, char * str1, char *str2);
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
int loop = 0;
int writeToPosition = 0;

int main(int argv, char ** argc)
{
    initialize();
    for (;;)
    {
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
        loop = 0;
    }
    printf("Closing miniShell\n");
    return 0;
}

int saveToFile(char ** parsed)
{

    int out = open(parsed[writeToPosition+1], O_RDWR|O_CREAT|O_APPEND, 0600);
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
    char ** aux = malloc(parsedItemsNo * sizeof(char *));
    for(i=0; i<writeToPosition; i++)
        aux[i] = parsed[i];
    if (fork() == 0)
    {
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
    return 1;
}

char *getInput()
{
    /* Lê e retorna caracteres do stdin. Por motivos de economia de memória, o programa salva a
    string do usuário de maneira dinâmicamente crescente. */
    char *input = (char *) malloc(BUFSIZ*sizeof(char)); //Aloca espaço para letras (tamanho máximo de buffer de stdin)
    char c = getchar(); // Recebe primeira letra..
    input[0] = c;       //..e inicializa input com ela.
    int i;

    /* Lê caracteres enquanto o usuário não pressiona Enter ou Ctrl-D e enquanto o comando de entrada não passa do tamanho máximo de buffer de stdin. */
    for(i=1; (i < BUFSIZ - 2) && ((c = getchar()) != '\n') && (c != EOF); ++i)
        input[i] = c;    //Insere letra no vetor

    //input = realloc(input, (i+1)*sizeof(char)); // Aloca espaço para terminador de string
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

int run(char ** parsed) // EM TESTE
{
    if (findPipe(parsed))
      return pipedCommand (parsed);

    else if(findRedirectToFile(parsed))
    {
        return saveToFile(parsed);
    }

    else
    {  // Caso não  haja nenhum pipe
        printf("Entrando em simpleCommand\n");
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
        else
            return simpleCommand(parsed);
    }
}

int simpleCommand(char ** parsed)
{
    pid_t testPID; // valor para teste pai/filho

    //printf("Entrou aqui?  %s \n", parsed[1]);
    testPID = fork();
    int status;
    if ( testPID == 0 )
    {
        execvp(parsed[0], parsed);
        printf("%s: comando não encontrado\n", parsed[0]);
    }
    else if (testPID < 0)
    {
        printf("Erro ao produzir fork.");
        return -1;
    }
    else
    {
        waitpid(-1, &status, WUNTRACED);
    }

    return 1; // sucesso
}

    // Obtem comandos auxiliares para execução em pipedCommand()
void getAuxCommand(char ** parsed, char ** aux)
{
    int i, c;
    if (loop == 0)
    {
        for(c = 0; c < pipePositions[0]; c++) // Copia parsed até chegar no primeiro "|"
        {
            aux[c] = parsed[c];
            //fprintf(stderr, "aux[%d]: %s\n", c, aux[c]);
           fprintf(stderr, "aux[%d]: %s\n", c, aux[c]);
        }
        fprintf(stderr, "loop: %d", loop);
        aux[c+1] = NULL;
        return;
    }
    else
    {
        c = 0;
        for(i=pipePositions[loop-1]+1;  i<pipePositions[loop];  i++)
            {
                  aux[c] = parsed[i];
                  fprintf(stderr, "aux[%d]: %s\n", c, aux[c]);
                  c++;
            }
        aux[c+1] = NULL;
    }
    fprintf(stderr, "loop: %d", loop);
}

    //Executa comandos com pipeline.
int pipedCommand(char ** parsed)
{
    pid_t pid;
    int fileDescriptor[2];
    if (pipe(fileDescriptor) == -1){
        fprintf(stderr, "Falha na criação de pipe.\n");
        return 0;
    }
    pid = fork();
    if(pid == 0)
    {
        //close(STDOUT_FILENO);
        dup2(fileDescriptor[WRITE_END], WRITE_END);
        close(fileDescriptor[READ_END]);
        close(fileDescriptor[WRITE_END]);
        char ** aux  = malloc(parsedItemsNo*sizeof (char *));

        getAuxCommand(parsed, aux);
        printf("Depois do pipe, primeiro: %s", aux[0]);
        printf("Depois do pipe, segundo: %s", aux[3-2]);
        execvp(aux[0], aux);
        fprintf(stderr, "%s:Comando não encontrado.\n", aux[0]);
        free(aux);
        return 0;
    }
    else{
        pid = fork();
        if(pid == 0)
        {
            loop++;
            dup2(fileDescriptor[READ_END], READ_END);
            close(fileDescriptor[WRITE_END]);
            close(fileDescriptor[READ_END]);
            char ** aux  = malloc(parsedItemsNo*sizeof (char *));
            getAuxCommand(parsed, aux);
            printf("Depois do pipe, primeiro: %s", aux[0]);
            printf("Depois do pipe, segundo: %s", aux[3-2]);
            execvp(aux[0], aux);
            fprintf(stderr, "%s: comando não encontrado.\n", aux[0]);
            free(aux);
            return 0;
        }
        else
        {
            int status;
            close(fileDescriptor[READ_END]);
            close(fileDescriptor[WRITE_END]);
            wait(0);
            wait(0);
            //loop = loop + 2;
            // Ideia: dar loop++; aqui ??
        }
    }
    int i;
    for(i=0; i<10; i++)
        pipePositions[i] = 0;
    return 2;
}
    /* Encontra pipes no comando lido, salvando suas posições em pipePositions */
int findPipe(char ** parsed) // TODO: encontrar mais de 2 forks
{
    int i, j, c=0, nPipes=0;
    for(i=0; i<parsedItemsNo; i++) // Varre os comandos lidos procurando por um caracter "|"
        if(strcmp(parsed[i], "|")==0)
        {
            for(j=0; j<11; j++)
                if (pipePositions[c]==0) // Se a posição atual ainda .TODO
                {
                    nPipes++;
                    pipePositions[c] = i;
                    printf("pipe em %d", i);
                    c++;
                    break;
                }
           }
    if(nPipes > 0)
    {
       pipePositions[c] = parsedItemsNo;
    }
    for(i=0; i<=c; i++)
        printf("a %d\n", pipePositions[i]);

    printf("nPipes: %d\n", nPipes);

    if(nPipes > 0)
        return true; // Retorna verdadeiro se algum "|" foi encontrado, falso caso contrário.
    else
        return false;
}

void typePrompt()
{
    printf(colorBlue "%s@%s:~/" colorRed "%s" colorBlue "$ " colorReset, username, hostname, currentDirName);
}

    /* muda o diretório do processo pai */
int changeDir(char ** parsed) // currentDirName deve ser modificado
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
            strcat(parsed[1], "\\ ");
            strcat(parsed[1], parsed[i]);
            printf("Novo parsed[1]: %s, quando i=%d\n", parsed[1], i);
        }

        strcat(parsed[1], "/");

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
    printf("\n   -------- miniShell --------\n\nVinícius R. Miguel & Gustavo B. de Oliveira\ngithub.com/vrmiguel/miniShell -- Unifesp -- Março de 2019\n\n");
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
