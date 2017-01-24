#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_BUF 1024

void LetsClean(char *msg,int no);
void writeToPipe(char *msg);

static volatile int keeprunning=1;


void sigintHandler(int d){
	keeprunning=0;
}

//Lukee putkesta merkkijonon ja kirjoittaa lokitiedostoon, käyttää file lockingia
int pipeToLog(void){

	struct flock fl;

	fl.l_type   = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start  = 0;
	fl.l_len    = 0;
	fl.l_pid    = getpid();

	FILE *f;
	char * myfifo="/tmp/myfifo";
	int fd,num;
	char buf[MAX_BUF];

	f=fopen("log.txt","a");
	int fd2=fileno(f);
	//File locking päälle
	fcntl(fd2,F_SETLKW,&fl);

	if((fd=open(myfifo,O_RDONLY))<0){
		perror("parent - open");
	}


	if((num=read(fd,buf,MAX_BUF))<0){
		perror("parent - read");
	}else{

		fprintf(f,"%s\n",buf);
		char substr[5];
		int pos=strlen(buf)-4;

		strncpy(substr,buf+pos,5);

		if(strcmp(substr,"Exit")==0){
			return 0;
		}
	}
	//File locking pois päältä
	fl.l_type=F_UNLCK;
	fcntl(fd2,F_SETLKW,&fl);

	close(fd);
	fclose(f);
	return 1;
}

/*Yksinkertainen funktio, jonka avulla aliprosessi lukee jatkuvasti putkeen
*ilmestyviä merkkijonoja ja kirjoittaa ne lokiin
*/
void pipeReader(void){
	//nimetty putki, alustus
	remove("/tmp/myfifo");
	char * myfifo="/tmp/myfifo";
	if(mkfifo(myfifo, 0666)<0){
		perror("mkfifo");
	}

	int running=1;
	while(running==1){
		if(pipeToLog()==0){
			running=0;
		}
		sleep(1);
	}
	unlink(myfifo);
}

int main(void) {
	void sigintHandler(int sig);
	struct sigaction sa;

	sa.sa_handler=sigintHandler;
	sa.sa_flags=0;
	sigemptyset(&sa.sa_mask);

	if(sigaction(SIGINT,&sa,NULL)==-1){
		perror("signal error");
		exit(1);
	}

	int running=1;
	int children=0;
	pid_t pid;
	pid=1;
	char text[128];

	//Luodaan ensimmäinen aliprosessi joka vastaa lokin kirjoituksesta
	pid=fork();
	if(pid==0){

		pipeReader();
		return EXIT_SUCCESS;
	}

	sleep(1);
	writeToPipe("Program start");

	/*Alla oleva looppi kysyy tiedostojen nimiä käyttäjältä, luo aliprosesseja
	 * käsittelemään niitä, sekä varmistaa toimivan poistumisen ohjelmasta
	 *
	 */
	while (running==1){
		if(keeprunning==0){
			printf("CTRL - C -signal received, terminating\n");
			running=0;
			break;
		}
		printf("Enter files name to clean it (type 'exit' to exit):\n");

		if(fgets(text,sizeof(text),stdin)==NULL){
			perror("fgets")
;		}
		if(strlen(text)>4){
			char tmp[4];
			strncpy(tmp,text,4);
			tmp[4]=0;
			if(strcmp(tmp,"exit")==0){
				printf("Exiting main\n");
				running=0;
				break;
			}
		}else if(strlen(text)==1){
			continue;
		}
		children++;
		pid=fork();
		if(pid==0){
			strtok(text,"\n");
			LetsClean(text,children);
			break;
		}

	}
	if(pid!=0){
		writeToPipe("Exit");

		printf("Waiting for child processes\n");
		wait(NULL);


		printf("Shutting down\n");

	}
	return EXIT_SUCCESS;
}


