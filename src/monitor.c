#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#define MAX_PROGRAMS 20
#define PROGRAM_LENGTH 256
#define MAX_BUFFER 4096
#define MIN_BUFFER 10

void add_program(char programs[][PROGRAM_LENGTH],char* program, int num_programs){
    if (num_programs < MAX_PROGRAMS){
            strcpy(programs[num_programs],program);
    }
}

void remove_programs(char programs[][PROGRAM_LENGTH],char* target_pid, int num_programs){
    for(int  i =0; i< num_programs; i++){
        char program_copy[PROGRAM_LENGTH];
        strcpy(program_copy, programs[i]);
        char* pid = strtok(program_copy,";");
        if (strcmp(pid,target_pid)==0){
            for(int j =i; j<num_programs-1; j++){
                strcpy(programs[j],programs[j+1]);
            }
            num_programs--;
            break;
        }
    }
}

int find_pid(char programs[][PROGRAM_LENGTH], char* target_pid, int num_programs){
    for(int  i =0; i< num_programs;i++){
        char program_copy[PROGRAM_LENGTH];
        strcpy(program_copy, programs[i]);
        char* pid = strtok(program_copy, ";");
        if (strcmp(pid, target_pid)==0){
            return 1;
        }
    }
    return 0;
}

int find_status(char program[], char* target_status){
    char program_copy[PROGRAM_LENGTH];
    strcpy(program_copy, program);
    strtok(program_copy, ";");
    strtok(NULL, ";");
    strtok(NULL, ";");
    strtok(NULL, ";");
    char* status =strtok(NULL,";");
    if (strcmp(status, target_status)==0){
        return 1;
    }
    return 0;   
}

char* get_program_pid(char* buffer){
    char* copy = strdup(buffer);
    char* pid = strtok(copy, ";");
    char* pid_copy = strdup(pid);
    free(copy);
    return pid_copy;
}

char* get_program_name(char* buffer){
    char* copy = strdup(buffer);
    char* name = strtok(copy, ";");
    name = strtok(NULL, ";");
    char* program_name = strdup(name);
    free(copy);
    return program_name;
}

char* get_program_time_inicio(char* buffer){
    char* copy = strdup(buffer);
    char* token = strtok(copy, ";");
    token = strtok(NULL, ";");
    token = strtok(NULL, ";");
    char* time_inicio = strdup(token);
    free(copy);
    return time_inicio;
}

void printfProgramsList(char programs[][PROGRAM_LENGTH], int num_programs){
    printf("Program list:\n");
    printf("+----------+----------------+--------------+--------------+-----------*\n");
    printf("|   PID    !  Program name  | Time inicio  |  Time final  |   Status  |\n");
    printf("+----------+----------------+--------------+--------------+-----------*\n");
    for(int i = 0; i < num_programs;i++){
        char program_copy[PROGRAM_LENGTH];
        program_copy[0] = '\0';
        strcpy(program_copy, programs[i]);
        char* pid= strtok(program_copy, ";");
        char* program_name= strtok(NULL, ";");
        char* time_inicio= strtok(NULL, ";");
        char* time_fim= strtok(NULL, ";");
        char* status= strtok(NULL, ";");
        printf("| %-4s | %-16s | %-16s | %-16s | %-9s |\n", pid, program_name, time_inicio, time_fim, status);
    }
    printf("+----------+----------------+--------------+--------------+-----------*\n");
}
 int exec(char mainBuffer[],char programaList[][PROGRAM_LENGTH],int num_program){
    char buffer[MAX_BUFFER];
    char myfifo2[MIN_BUFFER];
    snprintf(myfifo2,MIN_BUFFER,"../tmp/%s",mainBuffer);
    int fd_pid = open (myfifo2,O_RDONLY);
    if (fd_pid == -1){
        perror("Open specific pipe");
        return 1;
    }
    ssize_t n2 =0;
    n2 = read (fd_pid,buffer,sizeof(buffer));
    if (n2>0){
        if (find_pid(programaList, mainBuffer, num_program)==1){
            remove_programs(programaList, mainBuffer, num_program);
            num_program++;
        }
        add_program(programaList,buffer,num_program);
        num_program++;
        printf("\nAdded/changed a program\n");
        printfProgramsList(programaList,num_program);
    }
    if(n2==-1){
        perror ("read from specific pipe");
        close(fd_pid);
        return 1;
    }else if (n2==0){
        printf("Reached end of file.\n");
        close(fd_pid);
        return 0;
    }
    return num_program;
 }

int status (char mainBuffer[], char programaList[][PROGRAM_LENGTH], int num_program){
    struct timeval time_now;
    char statusBuffer[MAX_BUFFER];
    char statusBuffer2[MAX_BUFFER];
    int fd_status= open("../tmp/status",O_WRONLY);
    if (fd_status==-1){
        perror ("open status pipe");
        return 1;
    }
    int counter =0;
    char estado[MIN_BUFFER];
    strcpy(estado,"executing\0");
    for (int z = 0; z<num_program; z++){
        if (find_status(programaList[z],estado)==1){
            char pid_status[MIN_BUFFER];
            strcpy(pid_status, get_program_pid(statusBuffer));
            char name_status[MIN_BUFFER];
            strcpy(name_status, get_program_name(statusBuffer));

            char time_inicio_str_status[MIN_BUFFER];
            strcpy(time_inicio_str_status, get_program_pid(statusBuffer));
            long time_start = strtol(time_inicio_str_status,NULL,10);
            gettimeofday(&time_now,NULL);
            long time_spent=((time_now.tv_sec*1000000)+time_now.tv_usec)-time_start;
            char time_spent_str_status[MIN_BUFFER];
            sprintf(time_spent_str_status,"%ld",time_spent/1000);
            snprintf(statusBuffer2,MAX_BUFFER,"PID:%s |NAme:%s |Time executing:%sms \n", pid_status,name_status,time_spent_str_status);
            int n3 = write (fd_status,statusBuffer2,sizeof(statusBuffer2));

            if (n3==-1){
                perror("write to status pipe");
                close(fd_status);
                return 1;
            } else if (n3< sizeof(statusBuffer2)){
                printf("partial write: only wrote %d out of %zd bytes\n", n3, sizeof(statusBuffer2));
            }
            counter++;
            statusBuffer2[0] = '\0';

        }
    }  
    if (counter==0){
        strcpy(statusBuffer, "de momento nao programas em executçao");
        int n3= write(fd_status, statusBuffer,sizeof(statusBuffer));
        if (n3==-1){
            perror("Write to stutus pipe");
            close(fd_status);
            return 1;
        }
    } 
    close(fd_status);
    return 0;
}   

int main(){
    char programaList[MAX_PROGRAMS][PROGRAM_LENGTH];
    int num_program=0;
    char* myfifo = "../tmp/fifo";
    mkfifo(myfifo,0666);
    char mainBuffer[MIN_BUFFER];
    ssize_t n1;
    while(1){
        int fd = open ("../tmp/fifo",O_RDONLY);
        if (fd==-1){
            perror("Open default pipe");
            close(fd);
            return 1;
        }
        n1= read (fd,mainBuffer,sizeof(mainBuffer));
        if(n1==-1){
            perror("Read pipe pipe");
            close(fd);
            return 1;
        }else if (n1==0){
            close(fd);
        }else{
            close(fd);
            if (strcmp(mainBuffer,"status\0")==0){
                status(mainBuffer, programaList,num_program);
            }else {
                int temp=exec(mainBuffer, programaList,num_program);
                num_program = temp;
            }
        }
    }
    return 0;
}