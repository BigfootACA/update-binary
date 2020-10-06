#if 0
${CC:-aarch64-linux-gnu-gcc} "${0}" -O2 -Wall -Wextra -Werror -lzip -lz ${CFLAGS} ${LDFLAGS} ${LIBS} ${@} -s -static -o "${0:0:-2}"
exit
#endif
/* BigfootACA Android Recovery update-binary */
/* BigfootACA <bigfoot@classfun.cn> */
/* 2020/10/06 v0.0.1 */
#include<zip.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<string.h>
#include<unistd.h>
#include<dirent.h>
#include<limits.h>
#include<libgen.h>
#include<sys/stat.h>
#include<sys/wait.h>
#define BUF_SIZE 1024
#define BASH "bash"
#define SCRIPT "updater.sh"
#define INSTALL "/tmp/install"
#define PATH INSTALL"/bin"
#define BASH_PATH PATH"/"BASH
#define SCRIPT_PATH PATH"/"SCRIPT
#define LOG_PATH "/cache/installer-uos-sagit.log"
#define PREFIX "ui_print "
#define INVAL "Invalid arguments.\n"
static int exit_perror(char*errors,int err){
	perror(errors);
	exit(err);
	return err;
}
static int exit_printf(int err,int fd,char*format,...){
	va_list a;
	char buff[PATH_MAX*4]={0};
	va_start(a,format);
	int r=vsnprintf(buff,(PATH_MAX*4)-1,format,a);
	va_end(a);
	write(fd,buff,r);
	return err;
}
static int write_zip_file(zip_file_t*en,zip_uint64_t size,char*file,char*buff,int buff_len){
	zip_uint64_t len=0;
	int read;
	int fd;
	if((fd=open(file,O_CREAT|O_WRONLY))<0)return fd;
	while(len<size){
		memset(buff,0,buff_len);
		if((read=zip_fread(en,buff,BUF_SIZE))==-1)break;
		write(fd,buff,read);
		len+=read;
	}
	close(fd);
	return len;
}
static void ui_print(char*content){
	if(!content)return;
	fprintf(stdout,PREFIX"%s\n",content);
	fflush(stdout);
}
static int uncompress(char*file){
	int read;
	int err=0;
	char*package=getenv("PACKAGE");
	zip_t*z;
	zip_stat_t stat;
	zip_file_t*en;
	zip_uint64_t len=0;
	memset(&stat,0,sizeof(stat));
	if(!package)return exit_printf(10,STDERR_FILENO,"Environment variable PACKAGE not set.\n");
	if(access(package,R_OK)!=0)return exit_perror("ZIP file",11);
	if((z=zip_open(package,ZIP_RDONLY,&err))==NULL)return exit_printf(12,STDERR_FILENO,"Failed to open zip file: %d\n",err);
	if((en=zip_fopen(z,file,0))!=NULL&&zip_stat(z,file,0,&stat)==0){
		char buff[BUF_SIZE]={0};
		while(len<stat.size){
			memset(buff,0,BUF_SIZE);
			if((read=zip_fread(en,buff,BUF_SIZE-1))==-1)break;
			if(write(STDOUT_FILENO,buff,read)<=0)break;
			len+=read;
		}
		zip_fclose(en);
		zip_close(z);
		return 0;
	}else{
		zip_close(z);
		return exit_printf(12,STDERR_FILENO,"Failed to read %s\n",file);
	}
}
static void redirect_stdio(int fd){
	close(STDIN_FILENO);close(STDOUT_FILENO);close(STDERR_FILENO);
	dup2(fd,STDIN_FILENO);dup2(fd,STDOUT_FILENO);dup2(fd,STDERR_FILENO);
}
static int updater_main(char**argv){
	char*pkg=INSTALL,*path=PATH;
	mkdir(pkg,0755);
	mkdir(path,0755);
	if(access(pkg,F_OK)!=0)return exit_perror(pkg,4);
	if(access(argv[3],R_OK)!=0)return exit_perror("ZIP file",5);
	zip_t*z;
	int err=0;
	if((z=zip_open(argv[3],ZIP_RDONLY,&err))==NULL)return exit_printf(6,STDERR_FILENO,"Failed to open zip file: %d\n",err);
	zip_int64_t num=zip_get_num_entries(z,0);
	char name[PATH_MAX]={0},dirs[PATH_MAX]={0},buff[BUF_SIZE]={0};
	zip_file_t*en;
	zip_stat_t stat;
	for(zip_int64_t i=0;i<num;i++){
		memset(&stat,0,sizeof(stat));
		if(zip_stat_index(z,i,0,&stat)!=0)continue;
		if(stat.size>0x400000)continue;
		memset(&name,0,PATH_MAX);
		memset(&dirs,0,PATH_MAX);
		snprintf(name,PATH_MAX-1,"%s/%s",pkg,stat.name);
		strncpy(dirs,name,PATH_MAX);
		dirname(dirs);
		if(access(dirs,F_OK)!=0)mkdir(dirs,0755);
		if((en=zip_fopen_index(z,i,0))==NULL)continue;
		write_zip_file(en,stat.size,name,buff,BUF_SIZE-1);
		zip_fclose(en);
		chmod(name,0755);
	}
	zip_close(z);
	char*bash=BASH_PATH,*script=SCRIPT_PATH;
	if(access(bash,X_OK)!=0)return exit_perror(BASH,7);
	if(access(script,R_OK)!=0)return exit_perror("script",8);
	setenv("PATH",path,1);
	setenv("INSTALLER",pkg,1);
	setenv("UPDATER",argv[0],1);
	setenv("PACKAGE",argv[3],1);
	setenv("LOG",LOG_PATH,1);
	setenv("SHELL",bash,1);
	printf(PREFIX"Execute updater script.\n");
	if(execl(bash,BASH,script,NULL)!=0)return exit_perror("exec",9);
	return 0;
}
static int max(int a,int b){return a<b?b:a;}
static int updater(char**argv){
	if(argv[1][1]!='\0')return exit_printf(13,STDERR_FILENO,INVAL);
	else switch(argv[1][0]){
		case '1':case '2':case '3':break;
		default:return exit_printf(14,STDERR_FILENO,INVAL);
	}
	int log=open(LOG_PATH,O_CREAT|O_WRONLY);
	int fd[2],f;
	if((f=atoi(argv[2]))>2)redirect_stdio(f);
	close(0);
	if(pipe(fd)==-1)perror("pipe");
	pid_t pid=fork();
	if(pid==-1)return exit_perror(PREFIX"fork",9);
	else if(pid==0){
		close(fd[1]);
		char b[1],buff[BUF_SIZE]={0};
		int point=0;
		memset(buff,0,BUF_SIZE);
		while(read(fd[0],&b,1)>0)if(b[0]=='\n'||b[0]=='\r'||point>=BUF_SIZE-1){
			ui_print(buff);
			if(log>0){
				write(log,buff,strlen(buff));
				write(log,"\n",1);
			}
			memset(buff,0,BUF_SIZE);
			point=0;
		}else if(b[0]==0x0D){
			fdatasync(log);
			fflush(stdout);
		}else if(b[0]=='\t')for(int i=0,l=point+1;i<max(0,l%8)&&point<BUF_SIZE-1;i++)buff[point++]=' ';
		else buff[point++]=b[0];
		if(log>0)close(log);
		close(fd[0]);
		exit(0);
		return 0;
	}else if(pid>0){
		close(fd[0]);
		redirect_stdio(fd[1]);
		close(0);
		return updater_main(argv);
	}
	return 0;
}
int main(int argc,char**argv){
	if(getuid()!=0||geteuid()!=0)return exit_printf(2,STDERR_FILENO,"The program must run in root.\n");
	else if(access("/init.rc",F_OK)!=0)return exit_printf(3,STDERR_FILENO,"The program need android recovery.\n");
	else if(argc==3&&strcmp(argv[1],"getfile")==0)return uncompress(argv[2]);
	else if(argc==4)return updater(argv);
	else return exit_printf(3,STDERR_FILENO,INVAL);
}
