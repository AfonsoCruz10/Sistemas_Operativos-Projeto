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

//Adicionar todas a informações relativas ao comando exectudo( struct Cliente) à struct de dados
Cliente addstatus(Cliente p[],int id, char comando[],struct timeval inicial,struct timeval final,int status,int num){ 
    p[num-1].pid = id;
    strcpy(p[num-1].programas, comando);
    p[num-1].inicial = inicial;
    p[num-1].final= final;
    p[num-1].tipo_status=status;
    return p[num-1];
}

void printmonitor(Cliente c){

    printf("-------------------\n");
    printf("Novo comando:\n");
    printf("\tId do processo a executar: %d\n", c.pid);
    printf("\tComando a executar: %s\n", c.programas);
    printf("\tTempo em que começou a executar o comando: %ldms\n", c.inicial.tv_sec * 1000 + c.inicial.tv_usec /1000);
    printf("\tTempo em que acabou de executar o comando: %ldms\n", c.final.tv_sec * 1000 + c.final.tv_usec /1000 );
    printf("-------------------\n");

}

//Adiciona a nossa base de dados (p) o programa já executado e escreve no terminal do monitor os dados do mesmo
Cliente execu(int id, Cliente p[], int num){
    
    char fifoid_s[40];
	sprintf(fifoid_s, "../tmp/%d", id);

    //cria um fifo na pasta tmp com nome do pid 
    int fifo_id = mkfifo(fifoid_s,0666); 
    if( fifo_id == -1 && errno != EEXIST){
        perror("Erro ao criar o fifo execu");
        _exit(1);
    }
    Cliente c;

    //Abre o fifo para ler/receber as informações da estrutara Cliente
    int fd_exec = open(fifoid_s,O_RDONLY);
    if (fd_exec == -1){
        perror("Erro ao abrir o fd exec");
        _exit(-1);
    }
    if (read(fd_exec, &c, sizeof(c)) == -1){
        perror("Erro ao ler o fifo execu");
        _exit(1);
    }
    close(fd_exec);

    printmonitor(c);

    //Adiciona a nossa base de dados as informações de estrutura Cliente final
    p[num-1]= addstatus( p, c.pid, c.programas, c.inicial, c.final, c.tipo_status, num);
    
    return p[num-1];
}

void statuscomando(Cliente p[], int num){

    int fifo = mkfifo("../tmp/status",0666);  
    if( fifo == -1 && errno != EEXIST){
        perror("Erro ao criar o fifo no monitor");
        _exit(1);
    }

   

    char string[1000]="";
    char aux[100];
    int check=0;

    //dá print no terminal se o programa não estiver acabado
    int i = 0;
    while (i < num){
        if(p[i].tipo_status == 0){
            struct timeval finalstatus;
            gettimeofday(&finalstatus,NULL);

            long time =(finalstatus.tv_sec - p[i].inicial.tv_sec) * 1000 + (finalstatus.tv_usec - p[i].inicial.tv_usec) /1000;
            sprintf(aux,"%d  |  %s  |  %ldms\n", p[i].pid, p[i].programas, time);
            strcat(string,aux);
            check=1;               
        }
        i++;
    }

    if (check==0) strcat(string,"Sem programas programas em execucao!\n");

    // Abre o fifo status para escrever todas as informaçoes de todos os comanodas e enviar para o tracer 
    int fd_status = open("../tmp/status",O_WRONLY);
    if (fd_status==-1){
        perror("Erro ao abrir o fifo status");
        _exit(-1);
    }
    if( write(fd_status, string, strlen(string)) == -1){
        perror("Erro ao ao escrever status");
        _exit(-1);  
    }
    close(fd_status);
}

void statustime( Cliente p[], int num){ 

    int fifo = mkfifo("../tmp/status_time_read",0666);  
    if( fifo == -1 && errno != EEXIST){
        perror("Erro ao criar o fifo no monitor");
        _exit(1);
    }

    char string_pid[50];
    int args = 0 ;
    char *token;
    char *pids[40];

    int fd_statusread = open("../tmp/status_time_read", O_RDONLY);
    if(fd_statusread==-1){
        perror("Erro ao abrir status time read");
        _exit(-1);
    }
    if(read(fd_statusread, string_pid , sizeof(string_pid))==-1){
        perror("Erro ao ler status time read");
        _exit(-1);
    }
    close(fd_statusread);


    //separar a sstring de pids por ";" e adicionar num array pids
    token = strtok(string_pid, ";");
    // Percorrer todos os tokens
    while (token != NULL) {
        pids[args]=strdup(token);        
        // Obter o próximo token
        token = strtok(NULL, ";");
        args++;
    }

    int i=0, pid, j=0;  
    long soma =0;

    while(j < args && i < num){
        // String para inteiro
        pid = atoi(pids[j]);
        if (pid == p[i].pid){
            soma = soma + ((p[i].final.tv_sec - p[i].inicial.tv_sec) * 1000 + (p[i].final.tv_usec - p[i].inicial.tv_usec) /1000);
            j++;
            i=-1;
        }
        i++;
    }

    char resultado[100];
    sprintf(resultado,"Tempo total de execucao: %ldms \n",soma);
    
    //Cria um fifo para escrever o tempo total e enviar para tracer 
    int fd_statuswrite = open("../tmp/status_time_write", O_WRONLY);
    if(fd_statuswrite==-1){
        perror("Erro ao abrir status time write");
        _exit(-1);
    }
    if(write(fd_statuswrite, resultado , strlen(resultado))==-1){
        perror("Erro ao escrever status time write");
        _exit(-1);
    }
    close(fd_statuswrite);
}


//comando avançado que guarda informação de um programa acabado num ficheiro
void ficheiros(char* path, Cliente p[], int num){
    char print[100];
    int pid = p[num-1].pid;

    // Criar um diretorio
    int folder = mkdir(path,0777);
    if (folder == -1 && errno != EEXIST){
        perror("Erro ao criar diretorio");
        _exit(-1);
    }
    chmod(path, 0777);//concede total acesso ao diretorio criado

    if (p[num-1].tipo_status == 1){
        
        char pid_pipe_name[40];
        //transformar pid numa string
        sprintf(pid_pipe_name, "%d", pid);

        strcat(path,"/");
        strcat(path,pid_pipe_name);
        strcat(path,".txt");
        
        long time = (p[num-1].final.tv_sec - p[num-1].inicial.tv_sec) * 1000 + (p[num-1].final.tv_usec - p[num-1].inicial.tv_usec) /1000;
        sprintf(print,"Comando: %s\nDuracao do programa: %ldms", p[num-1].programas, time);

        int fd = open(path, O_CREAT|O_WRONLY,0666);
        if(fd==-1){
            perror("Erro ao abrir ficheiro pid");
            _exit(-1);
        }
        //Escrever no ficheiro a string print
        if((write(fd, print, strlen(print)))==-1){
            perror("Erro ao escrever para ficheiro pid");
            _exit(-1);
        }
        close(fd);

        // Redireciona o file discriptor para o standard output, ou seja, tudo o que for print vai escrever no ficheiro em vez do terminal  
        //dup2(fd,1);
        //printf("Comando: %s\n", p.programas[num-1]);
        //printf("Duracao do programa: %ldms\n", (p.final[num-1].tv_sec - p.inicio[num-1].tv_sec) * 1000 + (p.final[num-1].tv_usec - p.inicio[num-1].tv_usec) /1000);
        
        // inverter (stout fica apontar para o terminal) 
        //freopen("/dev/tty", "w", stdout);
    } 
}


int main(int argc, char* argv[]) {
    int num = 0;
    Cliente p [40];
    Cliente c;
    char* path;
    
    while (1){
        //Cria o fifo_main
	    int fifo = mkfifo("../tmp/fifo_main",0666);

        if( fifo == -1 && errno != EEXIST){
            perror("Erro ao criar o fifo no monitor\n");
            _exit(1);
        }
        char buffer[20];
        //Abre fifo e ler qual o tipo de comando a executar(se é "-u", "-l", "sair", "status", "status-time")
        int fd_main = open("../tmp/fifo_main",O_RDONLY);
        if (fd_main==-1){
            perror("Erro ao abrir o fifo monitor\n");
            _exit(-1);
        }else{
            if( read(fd_main, buffer, sizeof(buffer)) == -1){
                perror("Erro ao ler fifo_main\n");
                _exit(-1);
            }
        }
        close(fd_main);
        
        if (strcmp(buffer,"-u")==0){

            // Cria fifo
            int fifo = mkfifo("../tmp/fifo_pid",0666);
            if( fifo == -1 && errno != EEXIST){
                perror("Erro ao criar o fifo_pid\n");
                _exit(1);
            }

            //Abre o fifo para ler o pid do comando a executar
            int fd_serv = open("../tmp/fifo_pid",O_RDONLY);
            if (fd_serv==-1){
                perror("Erro ao abrir o fifo_pid");
                exit(-1);
            }
            if( read(fd_serv, &c, sizeof(c)) == -1){
                perror("Erro ao ler fifo_serv");
                exit(-1);
            }
            close(fd_serv);

            num++;
            //adicionar cliente inicial(comando ainda a exutar)
            p[num-1]= addstatus( p, c.pid, c.programas, c.inicial, c.final, c.tipo_status, num);

            //adicionar cliente final (comando ja executado)
            p[num-1] = execu(c.pid, p, num);
            if(argc == 2 &&  num != 0 ){ 
                path = strdup(argv[1]);  
                ficheiros(path, p, num);
            }
            
        } else if(strcmp(buffer,"-p")==0){


        } else if(strcmp(buffer,"status")==0){
            statuscomando(p, num);

        } else if(strcmp(buffer,"statustime")==0){
            statustime(p, num);

        } else if(strcmp(buffer,"sair")==0){
            return 0;
        }
        
    }	
        
}