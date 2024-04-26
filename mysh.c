/*
 * mysh.c
 * Authors:
 *    Cecilia Chen, Paprika Chen
 */
#define _POSIX_SOURCE 199309L
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "job.h"
#include "filesystem.h"

//#define MAX_INPUT_SIZE 128
#define DELIMITERS " ;\t\n"

#define EXIT_FLAG 0
#define SUSPEND_FLAG 1
#define STOP_FLAG 2
#define RESUME_FLAG 3
#define PRINT 0
#define WRITE 1
#define APPEND 2

char content[4096];

void rm_command(char *args[MAX_INPUT_SIZE]){
    if(args[1] ==NULL){
        strcat(content,"rm: missing operand\nTry 'rm --help' for more information.\n");
    }else if(strcmp(args[1],"--help")==0){
        strcat(content,"Usage: rm [OPTION]... [FILE]...\nRemove (unlink) the FILE(s).\n");
    }else{
        
        for(int i = 1;i<MAX_INPUT_SIZE;i++){
            if(args[i]==NULL){return;}
            f_delete(args[i]);
        }
    } 
}

void ls_command(char *args[MAX_INPUT_SIZE]){
    if(args[1]==NULL){
        struct dirent* cur = f_opendir(".");
        struct dirent* dirent = f_readdir(cur);
        while(dirent!=NULL){
            strcat(content,dirent->name);
            strcat(content,"    ");
            dirent = f_readdir(cur);
        }
        strcat(content,"\n");
        f_closedir(cur);
        return;
    }
    if(strcmp(args[1],"-l")==0){
        
    }else if(strcmp(args[1],"-F")==0){

    }
}

void mkdir_command(char *args[MAX_INPUT_SIZE]){
    if(args[1]==NULL){
        strcat(content, "mkdir: missing operand\nTry 'mkdir --help' for more information.\n");
    }else if(strcmp(args[1],"--help")==0){
        strcat(content, "Usage: mkdir DIRECTORY...\nCreate the DIRECTORY(ies), if they do not already exist.\n");
    }else{
        for(int i=1;i<MAX_INPUT_SIZE;i++){
            if(args[i]==NULL){return;}
            f_mkdir(args[i]);
        }
    }   
}

void pwd_command(){
   f_path(content);
   strcat(content,"\n");
}

void rmdir_command(char *args[MAX_INPUT_SIZE]){
    int flag = 0;
    if(args[1] ==NULL){
        strcat(content,"rmdir: missing operand\nTry 'rmdir --help' for more information.\n");
    }else if(strcmp(args[1],"--help")==0){
        strcat(content,"Usage: rmdir [OPTION]... DIRECTORY...\nCreate the DIRECTORY(ies), if they do not already exist.\n");
    }else{
        int i = 1;
        if(strcmp(args[1],"-r")==0){
            i=2;
            flag = 1;
        }
        for(;i<MAX_INPUT_SIZE;i++){
            if(args[i]==NULL){return;}
            f_rmdir(args[i],flag);
        }
    } 
}

void cd_command(char *arg){
    if(arg==NULL){
        return;
    }else if(strcmp(arg,"--help")==0){
        strcat(content,"cd: cd [path_dir]\nChange the shell working directory.\nChange the current directory to DIR.\n");
        return;
    }
    struct dirent* handle = f_opendir(arg);
    if(handle!=NULL){
        current_direct = handle;
    }
}

// parses and executes the given command line using a child process
void execute_command(char *command_line) {
    int run_in_background = 0;
    
    // check for exit command
    if (strcmp(command_line, "exit") == 0) {
        free(command_line);
        disk_close();
        exit(0);
    }

    // parse command line into command and arguments
    char *token = strtok(command_line, DELIMITERS);
    char *args[MAX_INPUT_SIZE];
    int arg_count = 0;
    
    // check for empty command
    if (token == NULL) {
        return;
    }

    char entire_command[MAX_INPUT_SIZE];
    strcpy(entire_command, token);

    // tokenize input
    while (token != NULL) {
        args[arg_count++] = token;
        token = strtok(NULL, DELIMITERS);

        // concatenate tokens into entire_command
        if (token != NULL) {
            strcat(entire_command, " ");
            strcat(entire_command, token);
        }
    } 

    // check if the last token is "&" and exclude it
    size_t command_len = strlen(entire_command);
    if (strcmp(entire_command + command_len - 1, "&") == 0) {
        entire_command[command_len - 1] = '\0';
    }

    args[arg_count] = NULL;

    // check if need to redirect
    
    int output_flag = -1;
    char* output;
    int new_i = 0;
    char *new_args[MAX_INPUT_SIZE];
    for(int i=0;i<arg_count;i++){
        if(strcmp(">>",args[i])==0){
            output_flag = APPEND;
            i = i+1;
            int len = strlen(args[i]);
            output = (char*) malloc(len+1);
            strcpy(output, args[i]);
            output[len]='\0';
        }else if(strcmp(">",args[i])==0){
            output_flag = WRITE;
            i = i+1;
            int len = strlen(args[i]);
            output = (char*) malloc(len+1);
            strcpy(output, args[i]);
            output[len]='\0';
        }else if(args[i][0]=='>'&&args[i][1]=='>'){
            output_flag = APPEND;
            int len = strlen(args[i]);
            output = (char*) malloc(len);
            strcpy(output, args[i]+1);
            output[len]='\0';
        }else if(args[i][0]=='>'){
            output_flag = WRITE;
            int len = strlen(args[i]);
            output = (char*) malloc(len);
            strcpy(output, args[i]+1);
            output[len]='\0';
        }else{
            new_args[new_i] = args[i];
            new_i++;
        }
    }
    new_args[new_i] = NULL;
    memset(content, '\0', sizeof(content));

    // implement jobs command
    if (strcmp(new_args[0], "jobs") == 0){
        printJobs(job_list);
        if(output_flag==-1){output_flag=0;}
    }
    else if(strcmp(new_args[0],"mkdir")==0){
        mkdir_command(new_args);
        if(output_flag==-1){output_flag=0;}
    }
    else if(strcmp(new_args[0],"rmdir")==0){
        rmdir_command(new_args);
        if(output_flag==-1){output_flag=PRINT;}
    }
    else if(strcmp(args[0],"cd") ==0){
        cd_command(new_args[1]);
        if(output_flag==-1){output_flag=PRINT;}
    }
    else if(strcmp(args[0],"ls")==0){
        ls_command(new_args);
        if(output_flag==-1){output_flag=PRINT;}
    }else if(strcmp(args[0],"pwd")==0){
        pwd_command();
        if(output_flag==-1){output_flag=PRINT;}
    }else if(strcmp(args[0],"rm")==0){
        rm_command(new_args);
        if(output_flag==-1){output_flag=PRINT;}
    }
    if(output_flag==WRITE){
        File* file = f_open(output,"w");
        f_write(file,content,strlen(content));
        f_close(file);
        free(output);
        return;
    }
    else if(output_flag==APPEND){
        File* file = f_open(output,"a");
        f_write(file,content,strlen(content));
        f_close(file);
        free(output);
        return;
    }
    else if(output_flag==PRINT){
        printf("%s",content);
        return;
    }
    
    // implement kill command
    if (strcmp(args[0], "kill") == 0){
        if (args[1] == NULL){
            printf("kill: usage: kill [pid]\n");
            return;
        }
        int index = 1;
        if (strcmp(args[1], "-9") ==0){
            index = 2;
        }
        if(args[index]==NULL){
            printf("kill:usage: kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec] \n");
            return;
        }
        int num = atoi(args[index]);
        if (num == 0 && args[index][0] != '0') {
            printf("kill: %s: arguments must be process\n",args[index]);
            return;
        }
        if(index == 2){
            kill((pid_t)num,SIGKILL);
        }else{
            kill((pid_t)num,SIGTERM);
        }
        return;
    }

    // implement fg command
    if (strcmp(args[0], "fg") == 0) {
    struct Job* jobToResume = NULL;
        // if a jobid is provided
        if (args[1] != NULL) {
            int jobId;
            if (args[1][0]=='%'){
                jobId = atoi(args[1]+1);
                if (jobId == 0 && args[1][1] != '0') {
                    printf("Invalid job ID: %s\n",args[1]+1);
                    return;
                }
            }else{
                jobId = atoi(args[1]);
                if (jobId == 0 && args[1][0] != '0') {
                    printf("Invalid job ID: %s\n",args[1]);
                    return;
                }
            }

            for (struct Job* job = job_list; job != NULL; job = job->next) {
                if (job->jobId == jobId) {
                    jobToResume = job;
                    break;
                }
            }

            if (!jobToResume) {
                printf("Job [%d] not found.\n", jobId);
                return;
            }

        }else {
            // if the jobid is not provided, find the last job
            jobToResume = tail;

            if (!jobToResume) {
                printf("No suspended job found to resume in the background.\n");
                return;
            }
        }
        jobToResume->status = RUNNING;
        pid_t fgPid = jobToResume->pid;
        printf("%s\n",jobToResume->command);
        // save current terminal settings
        struct termios savedSettings;
        if (tcgetattr(STDIN_FILENO, &savedSettings) == -1) {
            perror("tcgetattr");
            return;
        }

        // restore terminal settings
        if (tcsetattr(STDIN_FILENO, TCSADRAIN, &(jobToResume->setting)) == -1) {
            perror("tcsetattr");
            return;
        }

        // set terminal pgroup to fg pgroup
        if (tcsetpgrp(STDIN_FILENO, fgPid) == -1) {
            perror("tcsetpgrp");
            return;
        }

        // send SIGCONT signal if the job is stopped
        if (kill(-fgPid, SIGCONT) == -1) {
            perror("kill");
            return;
        }

        // wait for the foreground process to complete
        int status;
        if (waitpid(fgPid, &status, WUNTRACED) == -1) {
            perror("waitpid");
        }

        // set terminal pgroup back to the shell
        if (tcsetpgrp(STDIN_FILENO, getpgrp()) == -1) {
            perror("tcsetpgrp");
        }

        // restore original terminal settings
        if (tcsetattr(STDIN_FILENO, TCSADRAIN, &savedSettings) == -1) {
            perror("tcsetattr");
        }
        return;
    }

    // implement bg command
   if (strcmp(args[0],"bg") == 0) {
        struct Job* jobToResume = NULL;
        // if a jobid is provided
        if (args[1] != NULL) {
            int jobId;
            if (args[1][0]=='%'){
                jobId = atoi(args[1]+1);
                if (jobId == 0 && args[1][1] != '0') {
                    printf("Invalid job ID: %s\n",args[1]+1);
                    return;
                }
            }else{
                jobId = atoi(args[1]);
                if (jobId == 0 && args[1][0] != '0') {
                    printf("Invalid job ID: %s\n",args[1]);
                    return;
                }
            }
            for (struct Job* job = job_list; job != NULL; job = job->next) {
                if (job->jobId == jobId) {
                    jobToResume = job;
                    break;
                }
            }
            if (!jobToResume) {
                printf("Job [%d] not found.\n", jobId);
                return;
            }

            if (jobToResume->status != BLOCKED) {
                printf("Job [%d] is not suspended and cannot be resumed.\n", jobId);
                return;
            }
        }else {
        // if the jobid is not provided, find the last suspended job
            for (struct Job* job = tail; job != NULL; job = job->prev) {
                if (job->status == BLOCKED) {
                    jobToResume = job;
                    break;
                }
            }
            if (!jobToResume) {
                printf("No suspended job found to resume in the background.\n");
                return;
            }
        }
        if (kill(-(jobToResume->pid), SIGCONT) < 0) {
            perror("Error sending SIGCONT to job");
            return;
        }
        jobToResume->is_background = 1; // mark the job as running in the background
        //printf("Resumed job [%d] %s in the background.\n", jobToResume->jobId, jobToResume->command);
        return;
    }

    // check if the command is ended with '&'
    size_t len = strlen(args[arg_count-2]);
    if (len > 0 && args[arg_count-2][len - 1] == '&') {
        run_in_background = 1;
        args[arg_count-2][len-1] = '\0';
        if(len==1){args[arg_count-2]='\0';}
    }

    // start a child process to execute command
    pid_t pid = fork();

    if (pid == 0) { // child

        // set ignored signals to default
        signal(SIGINT,SIG_DFL);
        signal(SIGTSTP,SIG_DFL);
        signal(SIGTERM,SIG_DFL);
        signal(SIGTTIN,SIG_DFL);
        signal(SIGTTOU,SIG_DFL);
        signal(SIGQUIT,SIG_DFL);

        if (setpgid(0, 0) == -1) {
            perror("setpgid");
            exit(EXIT_FAILURE);
        }

        // search PATH and execute the command
        execvp(args[0], args);  
        perror("execvp fails"); 
        exit(EXIT_FAILURE);

    } else if (pid > 0){  // parent 
        addJob(run_in_background, entire_command, pid);

        if (setpgid(pid, 0) == -1) {
            perror("setpgid");
            exit(EXIT_FAILURE);
        }
        //需要储存terminal信息，把terminal给fg job
        if (run_in_background){
            tcsetpgrp(STDIN_FILENO, getpgrp());
        }else{
            tcsetpgrp(STDIN_FILENO, pid);
            // wait for the child to terminate
            int status;
            waitpid(pid, &status, WUNTRACED); 
            tcsetpgrp(STDIN_FILENO, getpgrp());
        }
         
    } else { // bad fork
        perror("fork fails");
        exit(EXIT_FAILURE);
    }
}

void child_handler(int signum, siginfo_t *info, void *context){

    pid_t pid = info -> si_pid; 
    //si_code in the info struct contains if the child is suspended, exited, or interrupted...
 	
   	if(info->si_code == CLD_EXITED){
        int status;
        waitpid(pid, &status, WNOHANG);
        add(pid,EXIT_FLAG);
    }
    if(info->si_code== CLD_KILLED){
        // avoid zombie process
        int status;
        waitpid(pid, &status, WNOHANG);
        add(pid,STOP_FLAG);
    }
    if(info->si_code == CLD_CONTINUED){
        add(pid,SUSPEND_FLAG);
	}
    if(info->si_code == CLD_STOPPED){
        kill(pid,SIGSTOP);
        add(pid,SUSPEND_FLAG);
   	}
}

//update job's status and termial setting
void updateJob(){

	while(revise_list !=NULL){
		pid_t pid = revise_list->pid ;
		int flag = revise_list->flag;
		if (flag == EXIT_FLAG || flag == STOP_FLAG){
            removeJob(pid);
        }else{
            struct Job* current = job_list;
            while (current != NULL && current->pid != pid) {
                current = current->next;
            }

            if (current == NULL) {
                printf("Job with PID %d not found\n", pid);
                return;
            }
            if(flag == SUSPEND_FLAG){
                current->status = BLOCKED;
                //current->setting = revise_list->setting;
            }
            if(flag == RESUME_FLAG){
                current->status = RUNNING;
            }
        }
        struct Revise* head = pop(&revise_list);
        free(head);
	}
}

int main() {
    if(disk_open("diskimage")!=1){
        printf("Cannot load diskimage\n");
        return -1;
    }
    char *input = (char *)NULL;

    // ignore useless signals
    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    signal(SIGTERM,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGTTOU,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);

    struct sigaction chldsa;
    chldsa.sa_sigaction = child_handler;
    sigemptyset(&chldsa.sa_mask);
	// block sigchld signals during the execution of the sigchld handler
	sigaddset(&chldsa.sa_mask, SIGCHLD); 
    chldsa.sa_flags = SA_RESTART|SA_SIGINFO;

    if (sigaction(SIGCHLD, &chldsa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
    while (1) {
        // reset or empty input each time
        if (input){
            free(input);
            input = (char *)NULL;
        }

        // get command
        input = readline("mysh> ");
        
        if (!input){
            printf("\n");
            break;
        }
        updateJob();
        if (*input){
            execute_command(input);
        }
    }
    
    disk_close();
    // cleanup 
    free(input);
    input = (char *)NULL;
    freeList();
    freeJobList();
    return 0;
}