#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#define MAX_ARGS 256
#define MAX_BUFFER 4096
#define MIN_BUFFER 10	

int execProg(char *program, int argc, char **args){

	pid_t pid;

	struct timeval time_inicio;
	struct timeval time_fim;

	int fd = open("../tmp/fifo", O_WRONLY);

	if (fd == -1){
		perror("open default pipe");
		return 1;
	}

	if ((pid = fork()) == 0) {

		execvp(program, args);
		perror("error executing the program");
		exit(1);
	}

	else if (pid > 0){
		char pid_pipe_name[MIN_BUFFER];
		sprintf(pid_pipe_name, "%d", pid);

		char myfifo[MIN_BUFFER];
		snprintf(myfifo, MIN_BUFFER, "../tmp/%.*s", MIN_BUFFER - 8, pid_pipe_name);

		if(mkfifo(myfifo, 0666)< 0) perror("mkfifo error");

		int n1 = write(fd,pid_pipe_name,sizeof(pid_pipe_name));

		if(n1 == -1){
			perror("write default Pipe");
			close(fd);
			return 1;
		}
		close(fd);

		char buffer[MAX_BUFFER];

		char pid_str[MIN_BUFFER];
		sprintf(pid_str, "%d", pid);

		char nome[MIN_BUFFER];
		strcpy(nome,args[0]);

		char time_inicio_str[MIN_BUFFER];
		gettimeofday(&time_inicio, NULL);
		sprintf(time_inicio_str,"%ld", time_inicio.tv_sec*1000000 + time_inicio.tv_usec);

		char time_fim_str[MIN_BUFFER];
		sprintf(time_fim_str, "%d", 0);

		char estado[MIN_BUFFER];
		strcpy(estado, "executing");

		snprintf(buffer, MAX_BUFFER, "%s;%s;%s;%s;%s;", pid_str, nome, time_inicio_str, time_fim_str,estado);

		int fd_pid = open(myfifo,O_WRONLY);
		if(fd_pid == -1){
			perror("open specific pipe");
			return 1;
		}

		int n2 = write(fd_pid,buffer, sizeof(buffer));

		if(n2 == -1){
			perror("write to specific pipe");
			close(fd_pid);
			return 1;
		}

		else{
			close(fd_pid);
			printf("o programa com o PID %d esta prestes a ser executado\n", pid );
		}
		sleep(10);

		int status;
		waitpid(pid, &status, 0);

		if(WIFEXITED(status)){
			int i = 0;
			int x = 0;
			while(buffer[i] && x<3){
				if(buffer[i]== ';') x++;
				i++;
			};
			buffer[i] = '\0';

			char time_fim_str_new[MIN_BUFFER];
			gettimeofday(&time_fim,NULL);
			sprintf(time_fim_str_new, "%ld", time_fim.tv_sec*1000000 + time_fim.tv_usec);

			char estado_new[MIN_BUFFER];
			sprintf(estado_new, "finished");

			snprintf(buffer + strlen(buffer), MAX_BUFFER - strlen(buffer), "%s;%s;", time_fim_str_new,estado_new);

			int fd = open("../tmp/fifo", O_WRONLY);

			if( fd == -1){
				perror("open default pipe");
				return 1;
			}


			n1 = write(fd,pid_pipe_name,sizeof(pid_pipe_name));

			if ( n1 == -1){
				perror("write to default pipe");
				close(fd);
				return 1;
			}

			close(fd);

			int fd_pid = open(myfifo, O_WRONLY);

			if(fd_pid == -1){
				perror("open specific pipe");
				return 1;
			}

			int n3= write(fd_pid, buffer, sizeof(buffer));

			if(n3== -1){
				perror("write to specific pipe");
				close(fd_pid);
				return 1;
			}
			close(fd_pid);

			long time_spent = ((time_fim.tv_sec)*1000000 + time_fim.tv_usec)- ((time_inicio.tv_sec)*1000000 + time_inicio.tv_usec);
			printf("O programa este em execucao durante %ld milisegundos. \n ", time_spent/1000);
			return WEXITSTATUS(status);

		} else{
			printf("error: program terminated abnormally\n");
			return 1;
		}
	}
	else{
		perror("fork");
		return 1;
	}
	return 1;
}

int execStatus(){

	char statusBuffer[MAX_BUFFER];
	int n_status2;

	int fd = open("../tmp/fifo", O_WRONLY);

	if( fd== -1){
		perror("open deafult pipe");
		return 1;

	}

	char * myfifoStatus = "../tmp/status";
	mkfifo(myfifoStatus, 0666);

	char myStatus[MIN_BUFFER];
	snprintf(myStatus, MIN_BUFFER, "%s", "status");

	int n_status = write(fd, myStatus, sizeof(myStatus));

	if(n_status == -1){
		perror("write to default pipe");
		close(fd);
		return 1;
	}
	close(fd);

	int fd_status = open("../tmp/status", O_WRONLY);

	if (fd_status == -1){
		perror("open status pipe");
		return 1;
	}
	while((n_status2 = read(fd_status,statusBuffer, sizeof(statusBuffer)))>0){
		printf("%s\n",statusBuffer);

	}
	close(fd_status);
	return 1;
}


int main(int argc, char const *argv[]){

	if((strcmp(argv[1], "execute")== 0) && (strcmp(argv[2], "-u")== 0)){
		char *program = argv[3];
		char *args[MAX_ARGS];
		int i;
		for(i= 0; i<MAX_ARGS-1 && i+3 < argc; i++){
			args[i] = argv[i + 3];
		}
		args[i] = NULL;
		execProg(program, argc-3,args);
	}

	else if
		(strcmp(argv[1], "status")==0){
			execStatus();
		}
}