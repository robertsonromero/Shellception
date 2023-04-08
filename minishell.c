#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> 
#include <linux/limits.h> //Path Max  
#include <string.h> // strings
#include <errno.h> // errors
#include <dirent.h> // directory stream
#include <sys/wait.h> //wait
#include <sys/stat.h> //stat
#include <linux/stat.h>
#include <fcntl.h>
#include <pwd.h> 
#include <signal.h>
#include <time.h>
#include <grp.h>
#define MAXLINE 1024    
#define GREEN "\033[32m"
#define BLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"

volatile sig_atomic_t interrupted;

void cd_cmd(char* currentdir, char** dest, int size){
    struct passwd *pwd = getpwuid(getuid());
    char* dir;
    if(pwd == NULL){
        fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
    }
   
    if(size > 2){
        fprintf(stderr, "Error: Too many arguments to cd.\n");
        return;
    }
    if(dest[1] == NULL || strcmp(dest[1], "~") == 0){
        dir = pwd->pw_dir;
    }
    else if(strpbrk("~",dest[1]) != NULL && strlen(dest[1]) > 1){
        char *space = strstr(dest[1],"~");
        if (space != NULL) {
            *space = '\0';
        }
        
        char buffer[PATH_MAX];
        snprintf(buffer,sizeof(buffer), "%s%s", pwd->pw_dir, space + 1);
        dir = buffer;
    } 
    else{
        dir = dest[1];
    }
     if (chdir(dir) != 0) {
        fprintf(stderr, "Error: Cannot change directory to '%s'. %s\n", dir, strerror(errno));
    }
    getcwd(currentdir,PATH_MAX);
}

int userin(char* command){
    int val;
    if(strcmp(command, "exit") == 0){
        val = 0;
    }
    else if(strcmp(command, "cd") == 0){
        val = 1;
    }
    else if(strcmp(command, "ls") == 0){
        val = 2;
    }
    else if(strcmp(command, "find") == 0){
        val = 3;
    }
   
    return val;
}

void find_cmd(char* directory, char* file,int size){
    struct passwd *pwd = getpwuid(getuid());
    DIR* dp;
    struct dirent* dirp;
    if(size < 3){
        fprintf(stderr, "Error: Not enough arguments.\n");
        return;
    }
    if(size > 3){
        fprintf(stderr, "Error: Too many arguments.\n");
        return;
    }
    if(strpbrk("~",directory) != NULL && strlen(directory) > 1){
        char *space = strstr(directory,"~");
        if (space != NULL) {
            *space = '\0';
        }
        
        char buffer[PATH_MAX];
        snprintf(buffer,sizeof(buffer), "%s%s", pwd->pw_dir, space + 1);
        directory = buffer;
    } 
    dp = opendir(directory);
    if(dp == NULL){
       fprintf(stderr, "Error: Cannot open directory. %s\n",strerror(errno));
    }
    while((dirp = readdir(dp)) != NULL){
        if((strcmp(dirp->d_name , ".") == 0) || (strcmp(dirp->d_name ,"..") == 0)){//ignoring ".."/"."  
            continue;
        } 
        char* newpath = (char*)malloc(sizeof(newpath) + sizeof(dirp->d_name) + 2);
        if(newpath == NULL){
            fprintf(stderr,"Error: malloc() failed. %s.\n",strerror(errno));
        }
        strcpy(newpath,directory);
        sprintf(newpath,"%s/%s",directory,dirp->d_name);
         if(strstr(dirp->d_name, file) != NULL){
            printf("%s\n",newpath);

        }
        
        free(newpath);
    }
    closedir(dp);
}

void ls_cmd(char* currdir){
    struct passwd *pwd = getpwuid(getuid());
    DIR* dp;
    struct dirent* dirp;
      if(strcmp(currdir, "~") == 0){
        currdir = pwd->pw_dir;
    }

    else if(strpbrk("~",currdir) != NULL && strlen(currdir) > 1){
        char *space = strstr(currdir,"~");
        if (space != NULL) {
            *space = '\0';
        }
        
        char buffer[PATH_MAX];
        snprintf(buffer,sizeof(buffer), "%s%s", pwd->pw_dir, space + 1);
        currdir = buffer;
    } 

    dp = opendir(currdir);
    if(dp == NULL){
        fprintf(stderr, "Error: %s.\n", strerror(errno));
        return;
    }

    while((dirp = readdir(dp)) != NULL){
        if(dirp->d_name == NULL) {
        continue;
        }
        if((strcmp(dirp->d_name , ".") == 0) || (strcmp(dirp->d_name ,"..") == 0)){//ignoring ".."/"."  
            continue;
        } 
        if(dirp->d_type == DT_DIR){
            printf("%s%s%s\n",GREEN, dirp->d_name, DEFAULT);
        }
        else{
            printf("%s\n",dirp->d_name);
        }
    }

    closedir(dp);
}

char* perm(char* file){ 
    struct stat filestat;
    if(stat(file, &filestat) != 0){
        return NULL;
    }
    char* permstr = (char*)calloc(10, sizeof(char)); 
     switch (filestat.st_mode & S_IFMT) {
        case S_IFBLK:  
            permstr[0] = 'b';            
            break;
        case S_IFCHR:  
            permstr[0] = 'c';       
            break;
        case S_IFDIR:  
            permstr[0] = 'd';              
            break;
        case S_IFIFO:  
            permstr[0] = 'p';             
            break;
        case S_IFLNK:  
            permstr[0] = 'l';                
            break;
        case S_IFREG:  
            permstr[0] = '-';           
            break;
        case S_IFSOCK: 
            permstr[0] = 's';                  
            break;
        default:       
            permstr[0] = '-';               
            break;
    }
    
    permstr[1] = (filestat.st_mode & S_IRUSR) ? 'r' : '-';
    permstr[2] = (filestat.st_mode & S_IWUSR) ? 'w' : '-';
    permstr[3] = (filestat.st_mode & S_IXUSR) ? 'x' : '-';
    permstr[4] = (filestat.st_mode & S_IRGRP) ? 'r' : '-';
    permstr[5] = (filestat.st_mode & S_IWGRP) ? 'w' : '-';
    permstr[6] = (filestat.st_mode & S_IXGRP) ? 'x' : '-';
    permstr[7] = (filestat.st_mode & S_IROTH) ? 'r' : '-';
    permstr[8] = (filestat.st_mode & S_IWOTH) ? 'w' : '-';
    permstr[9] = (filestat.st_mode & S_IXOTH) ? 'x' : '-';
    return permstr;
}


void sig_handler(int sig) {
    interrupted = 1;
    printf("\n");
   
}


int main(){
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = &sig_handler;
    if(sigaction(SIGINT, &action, NULL) == -1){
        fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
    }

    char cmdline[MAXLINE]; //command variable
    char currentpath[PATH_MAX]; //current path
    int flag = 0;
    int pid;
    int status;

    if(getcwd(currentpath, PATH_MAX) == NULL){ //get current working directory
        fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
    }
    while(1){
        printf("%s[%s]>%s ",BLUE, currentpath, DEFAULT);
        if(fgets(cmdline, MAXLINE, stdin) == NULL){
            if (interrupted == 1) {
            wait(NULL);
            interrupted = 0;
            continue;
        }
            fprintf(stderr, "Error: Failed to read from stdin. %s.\n", strerror(errno));
        }
        
        if (cmdline[strlen(cmdline) - 1] == '\n') {
            cmdline[strlen(cmdline) - 1] = '\0';
        }

        char *args[10];
        char *token = strtok(cmdline, " ");
        int i = 0;
        
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        
        args[i] = NULL; 

         

        flag = userin(args[0]);


        switch(flag){
            case 0:
                exit(EXIT_SUCCESS);
                return 0;
                break;
            case 1:
                cd_cmd(currentpath,args, i);
                break;
            case 2:
                if(args[1] == NULL){
                    ls_cmd(currentpath);
                }
                else{
                    ls_cmd(args[1]);
                }
                break;
            case 3:
                find_cmd(args[1],args[2], i);
                break;
                
            default:
                pid = fork();
                if(pid == -1){
                    fprintf(stderr,"Error: fork() failed. %s.\n", strerror(errno));
                }
                if (pid == 0){
                    
                    if(execvp(args[0], args) == -1){
                        if (interrupted == 1) {
                            wait(NULL);
                            interrupted = 0;
                            continue;
                        }
                        fprintf(stderr, "Error: exec() failed. %s.\n",strerror(errno));
                            exit(0);
                    }
                    EXIT_SUCCESS;
                }
                if(wait(&status) == -1){
                     if (interrupted == 1) {
                            interrupted = 0;
                            continue;
                        }
                    fprintf(stderr, "Error: wait() failed. %s.\n",strerror(errno));
                }
                break;
        }

    }
    
    return 0;
}
