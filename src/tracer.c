#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

typedef struct cliente{

    int pid;
    char programas[40];
    struct timeval inicial;
    struct timeval final;
    int tipo_status; // 0-ainda não acabado e 1-acabado

} Cliente;

//adiciona um cliente da esrrutura Cliente
Cliente addclient(int pid, char s[40], struct timeval tempo_inicial, struct timeval  tempo_final, int t){
    Cliente c;
    c.pid = pid;
    strcpy(c.programas,s);
    c.inicial = tempo_inicial;
    c.final = tempo_final;
    c.tipo_status=t;
    return c;
}

void executar(int argc, char* argv[]) {

    char *execvp_args[40];
    int args=0;
    int i=3;
    
    //Tempo inicial
    struct timeval start, end;
    gettimeofday(&start,NULL);

    // Cria uma lista de strings onde no indice 0 é o comando a executar, depois os argumentos e por ultimo NULL 
    while(i<argc){
        execvp_args[args]= strdup(argv[i]);
        i++;
        args++;
    }

    execvp_args[args] = NULL;
    
    //Cria um processo
    pid_t fork_ret = fork();

    if(fork_ret == -1){
        perror("Erro ao criar fork");
        exit(-1);
    }

    if(fork_ret == 0) {
        // Processo filho

        //pid do pai
        int pid = getppid();
        gettimeofday(&end,NULL);
        Cliente c2 = addclient(pid ,execvp_args[0],start,end, 0);
        
        int fd = open("../tmp/fifo_pid",O_WRONLY);
        if(fd == -1){
            perror("Erro em abrir fifo_pid");
            exit(-1);
        }
        if( write(fd, &c2, sizeof(c2)) ==-1){
            perror("Erro ao escrever em fifo_pid");
            exit(-1);
        }
        close(fd);

        printf("-------------------\n");
        printf("Processo id: %d\n", pid);
        printf("Resultado: \n\n");
        
        // Executar o comado dado pelo utilizador
        int err = execvp(execvp_args[0], execvp_args);

        if (err == -1){
            perror("Erro no exec");
            _exit(-1);
        }

    } else {
        //Processo pai 

        int pid=getpid();
        //transformar pid para string
        char fifoip_s[40];
		sprintf(fifoip_s, "../tmp/%d", pid);

        int wstatus;
        //epera pelo processo filho terminar
        wait(&wstatus);

        //Verifica se o processo filho devolveu algo e se o q devolveu é diferente de -1, é o q devolve se o execvp não executar corretamente 
        if(WIFEXITED(wstatus)) {
            int statuscode = WEXITSTATUS(wstatus);
            if (statuscode == -1){
                perror("Falha");
            }

            gettimeofday(&end,NULL);
            Cliente c2 = addclient(pid, execvp_args[0], start, end, 1);
            // Abre o fifo da pasta tmp com o nome do pid e manda para o servidor o um cliente que contem as informaçoes do comando, tempo, pid e status
            int fd_ip2 = open(fifoip_s,O_WRONLY);

            if(fd_ip2 == -1){
                perror("Erro ao abrir fd_pid2");
                exit(-1);
            }
            if(write(fd_ip2, &c2, sizeof(c2)) == -1){
                perror ("Error em escrever no fd_pid2");
                _exit(-1);
            }
            close(fd_ip2);
            
            printf("\nTempo de duracao da execucao do programa: %ldms\n",(c2.final.tv_sec - c2.inicial.tv_sec) * 1000 + (c2.final.tv_usec - c2.inicial.tv_usec) / 1000);
            printf("-------------------\n");

        }
    }
}

void execstatus(){
    char string[1000];

    //Abre fifo status e le a enviada pelo servidor
    int fd_status = open("../tmp/status", O_RDONLY);
    if(fd_status==-1){
        perror("Erro ao abrir status");
        _exit(-1);
    }
    if((read(fd_status, string , sizeof(string)))==-1){
        perror("Erro ao ler status time");
        _exit(-1);
    }
    close(fd_status);

    //escreve para o standar output a string recebida pelo monitor
    if (write(1, string , strlen(string))==-1){
        perror("Erro ao escrever em terminal");
        _exit(-1);
    }
}

//conecta strings separados por virgulas
void strpid(char* s,int c, char* v[]){
    int i=2;
    while ( i < c){
        strcat(s, v[i]);
        if (i != c - 1) strcat(s, ";");
        i++;
    } 
}

// comando avançado 
void statustime( int argc, char* argv[]){ 

    char string[50]="";
    char string_print[100];
    
    //conectas os pids separando-os por ponto e virgulas
    strpid(string, argc, argv);

    int fifo = mkfifo("../tmp/status_time_write",0666);  
    if( fifo == -1 && errno != EEXIST){
        perror("Erro ao criar o fifo ");
        _exit(1);
    }

    //Abre fifo status time e escreve uma string com os pid
    int fd_statuswrite = open("../tmp/status_time_read", O_WRONLY);
    if(fd_statuswrite==-1){
        perror("Erro ao abrir status time write");
        _exit(-1);
    }
    if(write(fd_statuswrite, string , strlen(string))==-1){
        perror("Erro ao escrever em status time write");
        _exit(-1);
    }
    close(fd_statuswrite);


    //Abre fifo 
    int fd_statusread = open("../tmp/status_time_write", O_RDONLY);
    if(fd_statusread==-1){
        perror("Erro ao abrir status time read");
        _exit(-1);
    }
    if(read(fd_statusread, string_print , sizeof(string_print))==-1){
        perror("Erro ao escrever status time read");
        _exit(-1);
    }
    close(fd_statusread);
    
    //Escrever no terminal a string print
    if(write( 1, string_print , strlen(string_print))==-1){
        perror("Erro ao escrever no terminal");
        _exit(-1);
    }
}
    

int main(int argc, char* argv[]) {

    //execução de programa inividual
    if (argc > 3 && (strcmp(argv[1],"execute")==0) && (strcmp(argv[2],"-u")==0) ){

        //Abre o fifo_main criado pelo servidor e manda para o servidor o tipo de execute, neste caso "-u"
        int fd_main = open("../tmp/fifo_main",O_WRONLY);

        if(fd_main == -1){
            perror("Erro em abrir fifo_main\n");
            exit(-1);
        }
        if(write(fd_main, "-u", strlen("-u")) == -1){
            perror("Erro ao abrir fifo_main");
            _exit(-1);
        }
        close(fd_main);

        executar(argc,argv);

    // não acabado    
    // execução de uma pipeline de programas
    }else if(argc > 3 && (strcmp(argv[1],"execute")==0) && (strcmp(argv[2],"-p")==0)){
        int fd_main = open("../tmp/fifo_main",O_WRONLY);

        if(fd_main == -1){
            perror("Erro em abrir fifo_main\n");
            exit(-1);
        }

        if(write(fd_main, "-p", strlen("-p")) == -1){
            perror("Erro ao abrir fifo_main");
            _exit(-1);
        }
        close(fd_main);

    //executar stautus
    }else if(argc > 1 && (strcmp(argv[1],"status")==0)){

        //Abre o fifo_main criado pelo servidor e manda para o servidor o tipo de execute, neste caso "status"
        int fd_main = open("../tmp/fifo_main",O_WRONLY);

        if(fd_main == -1){
            perror("Erro em abrir fifo_main\n");
            exit(-1);
        }
        if(write(fd_main, "status", strlen("status")) == -1){
            perror("Erro ao abrir fifo_main");
            _exit(-1);
        }
        close(fd_main);
        execstatus();

    //programa avançado
    //executar stautus-time
    }else if(argc > 2 && (strcmp(argv[1],"status-time")==0)){

        //Abre o fifo_main criado pelo servidor e manda para o servidor o tipo de execute, neste caso "status"
        int fd_main = open("../tmp/fifo_main",O_WRONLY);

        if(fd_main == -1){
            perror("Erro em abrir fifo_main\n");
            exit(-1);
        }
        if(write(fd_main, "statustime", strlen("statustime")) == -1){
            perror("Erro ao abrir fifo_main");
            _exit(-1);
        }
        close(fd_main);
        statustime(argc,argv);

    //desligar o servidor
    }else if(argc > 1 && (strcmp(argv[1],"sair")==0)){

        //Abre o fifo_main criado pelo servidor e manda para o servidor "sair" para desligar o servidor
        int fd_main = open("../tmp/fifo_main",O_WRONLY);

        if(fd_main == -1){
            perror("Erro em abrir fifo_main\n");
            exit(-1);
        }
        if(write(fd_main, "sair", strlen("sair")) == -1){
            perror("Erro ao abrir fifo_main");
            _exit(-1);
        }
        close(fd_main);
    }else{
        printf("Argumentos invalidos!!\n");
    }
	return 0;
}