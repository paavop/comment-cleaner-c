#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>


//Funktio tyhjien rivien poistamiseen tiedostosta
void removeBlanks(char *name){
	FILE *fp,*fp2;
	int p;
	fp=fopen(name,"r");
	fp2=fopen("tmp.txt","w");
	while((p=getc(fp))!=EOF){
		fputc(p,fp2);
		if(p==10){
			while((p=getc(fp))==10)
			{
			}
			if(p!=EOF){
				fputc(p,fp2);
			}
		}
	}
	fclose(fp);
	fclose(fp2);
	remove(name);
	rename("tmp.txt",name);
}

//Mahdollistaa viestin lähettämisen putkeen, eli käytännössä
//myös kirjoittamisen lokiin
void writeToPipe(char *msg){

	time_t timer;
	char buffer[109];
	struct tm* tm_info;

	time(&timer);
	tm_info=localtime(&timer);

	strftime(buffer,109,"[%Y-%m-%d %H:%M:%S] ", tm_info);

	strcat(buffer,msg);
	int fd;
	char * myfifo = "/tmp/myfifo";

	fd=open(myfifo,O_WRONLY);
	write(fd,buffer,109);

	close(fd);
}

/*Saa tiedoston nimen, yrittää avata ja puhdistaa tiedoston kommenteista
 * tiedoston auetessa. Kirjoittaa lokiin auetessaan ja valmistuessaan.
 */

void LetsClean(char *msg,int no){

	char msg2[80];
	char newname[80];
	snprintf(newname,sizeof(newname),"%s.clean",msg);
	snprintf(msg2,sizeof(msg2),"Child process %d has begun cleaning %s",no,msg);
	FILE * toread;
	char * line=NULL;
	size_t len=0;
	ssize_t read;

	toread=fopen(msg,"r");
	if(toread==NULL){
		snprintf(msg2,sizeof(msg2),"Child process %d can't read file %s",no,msg);
		writeToPipe(msg2);
		exit(EXIT_FAILURE);
	}
	writeToPipe(msg2);

	FILE * towrite;
	towrite=fopen(newname,"w");
	int copy=1;
	while((read=getline(&line,&len,toread))!=-1){

		char newline[100];
		memset(newline,0,sizeof(newline));
		int j;
		int i;
		for(i=1;line[i]!='\0';i++){
			j=i-1;

			if(line[j]=='/' && line[i]=='/'){

				break;

			}
			if(line[j]=='/' && line[i]=='*'){
				copy=0;
			}
			if(copy==0 && line[j]=='*' && line[i]=='/'){
				copy=1;
				continue;
			}
			if(copy==1){
				newline[j]=line[j];
			}
		}

		fprintf(towrite,"%s", newline);

		if(copy==1){
			fprintf(towrite,"\n");
		}





	}

	fclose(toread);
	fclose(towrite);
	if(line)
		free(line);
	removeBlanks(newname);

	snprintf(msg2,sizeof(msg2),"Child process %d has finished cleaning file %s",no,msg);
	writeToPipe(msg2);

}

