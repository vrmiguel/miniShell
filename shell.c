/*
    miniShell (https://github.com/vrmiguel/mini_shell)
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
#define RED_ANSI "\x1b[31m" //Código de escape ANSI para a cor vermelha
#define BLUE_ANSI "\x1b[34m" // Código de escape ANSI para a cor azul
#define RESET_ANSI   "\x1b[0m" // Código de escape ANSI para voltar a exibição para a cor padrão do stdout
#define WRITE_END 1
#define READ_END 0

//Protótipos
char *get_input(void);
char **parser(char *input);
void type_prompt(void);
int run(char ** parsed);
int change_dir(char ** parsed);
void initialize(void);
int simple_command(char ** parsed);
int piped_command(char ** parsed);
char * get_current_dir_name_only(void);
int  find_pipe(char ** parsed);
int  find_redirect_to_file(char ** parsed);

//Variáveis globais
char username[32]; // guardará o nome do usuário, com no máximo 32 caracteres, como estipulado pela biblioteca GNU C (glibc).
char hostname[64]; // guardará o nome do host, com no máximo 64 caracteres, como estipulado pela biblioteca GNU C (getconf HOST_NAME_MAX)
int parsed_items_qnt; // quantidade de palavras tokenizadas na string atual
char cwd[BUFSIZ]; /* Inicializa string para pasta atual, respeitando o limite máximo de caracteres que o stdin pode transferir */
char * current_dir_name;// Deverá ser inicializada em initialize() e então só modificada em change_dir()
int pipe_positions[10] = {0};
int n_pipes = 0;
int write_to_pos = 0;
int read_from_pos = 0;

char *get_input()
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

int main(int argv, char ** argc)
{
    initialize();
    for (;;)
    {
        type_prompt();
        int err_desc; // Variável para teste de retorno de execução | execution r
        char *input = get_input(); // Adquira input
        char ** parsed = parser(input);
        err_desc = run(parsed);

        free(input); //Libera memória alocada para input e parsed

        for(int i=0; i<parsed_items_qnt; i++)
            free(parsed[i]);
        free(parsed);
        if (!err_desc)
            break;
    }
    printf("Closing mini_shell\n");
    return 0;
}

int save_to_file(char ** parsed)
{
    int out = open(parsed[write_to_pos+1], O_RDWR|O_CREAT|O_TRUNC, 0600);
    if (out == - 1)
    {
        fprintf(stderr, "Erro em criação de arquivo \"%s\"", parsed[write_to_pos+1] );
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
        char ** aux = malloc(parsed_items_qnt * sizeof(char *));
        for(i=0; i<write_to_pos; i++)
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
    write_to_pos = 0;
    return 1;
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
    parsed_items_qnt = i+1; // Registra quantas palavras foram tokenizadas
    parsed[i+1] = NULL; // Necessário para ser argumento na família exec()

    return parsed;
}

int find_redirect_to_file(char ** parsed)
{
    int i;
    for (i=0;i<parsed_items_qnt;i++)
        if(strcmp(parsed[i], ">") == 0)
        {
            write_to_pos = i;
            return 1;
        }
    return 0;
}

int find_redirect_from_file(char ** parsed)
{
    int i;
    for (i=0;i<parsed_items_qnt;i++)
        if(strcmp(parsed[i], "<") == 0)
        {
            read_from_pos = i;
            return 1;
        }
    return 0;
}

int read_from_file(char ** parsed)
{
    int i;

    int open_desc= open(parsed[read_from_pos+1], O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (open_desc== -1)
    {
        fprintf(stderr, "Erro em leitura de arquivo \"%s\"", parsed[read_from_pos+1]);
        return 1;
    }

    int input = dup(fileno(stdin));
    if (dup2(open_desc, fileno(stdin)) == -1)
    {
        fprintf(stderr, "Problema em redirecionamento de stdin.\n");
        return 0;
    }

    char ** aux = malloc(read_from_pos * sizeof(char *));
    for(i=0; i<read_from_pos; i++)
        aux[i] = parsed[i];

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

    close(open_desc);
    dup2(input, fileno(stdin));
    close(input);
    read_from_pos = 0;
    return 1;
}

int run(char ** parsed)
{
    if (find_pipe(parsed))
        return piped_command(parsed);

    else if(find_redirect_to_file(parsed))
        return save_to_file(parsed);

    else if(find_redirect_from_file(parsed))
        return read_from_file(parsed);

    else
    {  // Caso não  haja nenhum pipe
        if(!strcmp(parsed[0] , "cd"))
            return change_dir(parsed);
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
            else return simple_command(parsed);
    }
}

int simple_command(char ** parsed)
{
    pid_t pid; // valor para teste pai/filho
    pid = fork();

    if(!strcmp(parsed[parsed_items_qnt-1], "&"))
    {
        printf("Background commands not supported.\nExiting.\n");
        return 0;
    }

    int status;
    if (pid == 0)
    {
        execvp(parsed[0], parsed);
        fprintf(stderr, "%s: comando não encontrado\n", parsed[0]);
    }
    else if (pid < 0)
    {
        printf("Erro ao produzir fork.");
        return 0;
    }
    waitpid(-1, &status, WUNTRACED);

    return 1; // sucesso
}

// Obtem comandos auxiliares para execução em piped_command()
void get_aux_command(char ** parsed, char ** aux, int loop)
{
    int i, c;
    if (!loop)
    {
        for(c = 0; c < pipe_positions[0]; c++) // Copia parsed até chegar no primeiro "|"
            aux[c] = parsed[c];
        //fprintf(stderr, "loop: %d", loop);
        aux[c+1] = NULL; // Argumentos para a família exec devem terminar com NULL
        return;
    }
    else
    {
        c = 0;
        for(i=pipe_positions[loop-1]+1;  i<pipe_positions[loop];  i++)
        {
            aux[c] = parsed[i];
            c++;
        }
        aux[c+1] = NULL;
    }
}

int piped_command(char** parsed){
    int n_commands = n_pipes + 1;
    int file_descriptor_array[10][2], i;

    if(!strcmp(parsed[parsed_items_qnt-1], "&"))
    {
        free(parsed[parsed_items_qnt-1]);
        parsed[parsed_items_qnt-1] = NULL;
        parsed_items_qnt--;
    }

    char ** aux = NULL;
    for(i=0; i<n_commands; i++)
    {
        if (aux != NULL)
            free(aux);
        char ** aux = malloc(parsed_items_qnt * sizeof(char*));
        get_aux_command(parsed, aux, i);
        if(i!=n_commands-1)
            if(pipe(file_descriptor_array[i])<0)
            {
                fprintf(stderr, "Erro na criação de pipe.\n");
                return 0;
            }
        if(fork()==0) // Primeiro processo filho
        {
            if(i!=n_commands-1)
            {
                dup2(file_descriptor_array[i][WRITE_END],STDOUT_FILENO);
                close(file_descriptor_array[i][READ_END]);
                close(file_descriptor_array[i][WRITE_END]);
            }
            if(i!=0)
            {
                dup2(file_descriptor_array[i-1][READ_END],STDIN_FILENO);
                close(file_descriptor_array[i-1][WRITE_END]);
                close(file_descriptor_array[i-1][READ_END]);
            }
            execvp(aux[0], aux);
            fprintf(stderr, "%s: Comando não encontrado.\n", aux[0]);
            return 0;
        }
        else
            if(i!=0)
            {
                close(file_descriptor_array[i - 1][READ_END]);
                close(file_descriptor_array[i - 1][WRITE_END]);
            }
    }
    for(i=0; i<n_commands; i++)
          wait(NULL);
    for(i=0; i<10; i++) // Reseta posições
        pipe_positions[i] = 0;
    free(aux);
    n_pipes = 0; // Reseta número encontrado de pipes
    return 1;
}

/* Encontra pipes no comando lido, salvando suas posições em pipe_positions */
int find_pipe(char ** parsed)
{
    int i, j, c=0;
    for(i=0; i<parsed_items_qnt; i++) // Varre os comandos lidos procurando por um caracter "|"
        if(strcmp(parsed[i], "|")==0)
            for(j=0; j<11; j++)
                if (pipe_positions[c]==0) // Se a posição atual ainda .TODO
                {
                    n_pipes++;
                    pipe_positions[c] = i;
                    c++;
                    break;
                }
    if(n_pipes > 0)
        pipe_positions[c] = parsed_items_qnt;

    return n_pipes > 0; // Retorna verdadeiro se algum "|" foi encontrado, falso caso contrário.
}

void type_prompt()
{
    printf(BLUE_ANSI "%s@%s:~/" RED_ANSI "%s" BLUE_ANSI "$ " RESET_ANSI, username, hostname, current_dir_name);
}

    /* muda o diretório do processo pai */
int change_dir(char ** parsed)
{
    if(parsed_items_qnt == 1 || !strcmp(parsed[1],"~")) // Comandos padrão para retorno para home
    { // TODO: mudar 'cd ..'
        /* procura o ambiente do processo chamante pela variável HOME para que o chdir possa mudar o ambiente da execução para pasta HOMEs */
        printf("Exibição de teste: chdir\n");
        chdir(getenv("HOME"));
        current_dir_name = "";
    }
    else
    {
        printf("parsed_items_qnt: %d\n", parsed_items_qnt);
        for(int i = 0; i<parsed_items_qnt; i++)
            printf("parsed[%d] = %s\n", i, parsed[i]);

        //printf("Iniciando concatenação\n\n");

        for(int i=2; i<parsed_items_qnt; i++)
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
            current_dir_name = get_current_dir_name_only(); // caso de sucesso
    }
    getcwd(cwd, BUFSIZ);//Grava o nome da pasta atual - TODO - desnecessário ?
    return 1;
}

void initialize()
{
    printf("\n   -------- miniShell --------\n\nVinícius R. Miguel, Lucas S. Vaz & Gustavo B. de Oliveira\ngithub.com/vrmiguel/miniShell -- Unifesp -- Março de 2019\n");
    printf("Digite \'help\' para obter ajuda.\n\n");
    getcwd(cwd, BUFSIZ); //Grava o nome da pasta atual
    //printf("CWD: %s\n", cwd);
    current_dir_name = get_current_dir_name_only(); // Formata a string da pasta atual para exibição em type_prompt()
    //printf("Current Dir: %s\n", current_dir_name);
    uid_t uid = geteuid(); // Adquire a ID efetiva do usuário que chamou o shell
    struct passwd *pw = getpwuid(uid); // Procura o UID no banco de dado de senhas e retorna um ponteiro para uma struct passwd
    strcpy(username, pw->pw_name); // Da struct passwd, salva-se o nome do usuário
    gethostname(hostname, 64);
    //printf("\n\n\n%s\n\n\n", pw->pw_name);
    //username = pw->pw_name;
}

/* Processa a variável cwd para posterior exibição correta em type_prompt()
    exemplo: "/home/vinicius/Downloads" --> "/Downloads"                 */
char * get_current_dir_name_only ()
{
    char aux_string[BUFSIZ];
    getcwd(aux_string, BUFSIZ); // Armazena diretório atual
    char *aux;
    char *token = strtok(aux_string, "/"); // Cria primeiro token (separador "/") e mantém o ponteiro aux_string em estado interno.
    for (;;) {
        token = strtok(NULL, "/"); // Adiciona mais um token do ponteiro em estado interno (aux_string).
        if (token == NULL)
            break;
        aux = token;
    }
    return aux;
}
