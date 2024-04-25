#include "job.h"
//#define _POSIX_SOURCE 199309L
//#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>


// function to create a new job
struct Job* createJob(int is_background, const char* command, pid_t pid) {
    struct termios setting;
    if(tcgetattr(STDIN_FILENO, &setting)< 0) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }
    struct Job* newJob = (struct Job*)malloc(sizeof(struct Job));
    if (newJob == NULL) {
        perror("Error creating job");
        exit(EXIT_FAILURE);
    }

    // assign vars
    newJob->jobId = ++jobCounter; 
    newJob->is_background = is_background;
    strncpy(newJob->command, command, sizeof(newJob->command) - 1);
    newJob->status = RUNNING; // default = RUNNING
    newJob->pid = pid;
    newJob->setting = setting;
    newJob->next = NULL;
    newJob->prev = NULL;
    if(is_background){printf("[%d]%d\n",newJob->jobId,(int)newJob->pid);}
    return newJob;
}

// function to add a job to the doubly linked list
void addJob(int is_background, const char* command, pid_t pid) {
    struct Job* newJob = createJob(is_background, command, pid);
    if (job_list == NULL) {
        // when list is empty
        job_list = newJob;
        tail = newJob; 
    } else {
        // when list is not empty
        tail->next = newJob;
        newJob->prev = tail;
        tail = newJob;
    }
}

// function to remove a job from the doubly linked list
void removeJob(pid_t pid) {
    struct Job* current = job_list;

    while (current != NULL && current->pid != pid) {
        current = current->next;
    }

    if (current == NULL) {
        printf("Job with ID %d not found\n", pid);
        return;
    }

    if (current->prev == NULL) {
        // If the job to be removed is the first one
        job_list = current->next;
        if (job_list != NULL) {
            job_list->prev = NULL;
        }
    } else {
        // If the job to be removed is not the first one
        current->prev->next = current->next;
        if (current->next != NULL) {
            current->next->prev = current->prev;
        }
    }

    if (current == tail) {
        // Update the tail if the removed job was the last one
        tail = current->prev;
    }

    // Update the jobid
    struct Job* update = current->next;
    while (update!=NULL){
        update->jobId--;
        update = update->next;
    }
    jobCounter--;
    free(current);
    //printf("Job with PID %d removed\n", pid);
}

// function to print the doubly linked list
void printJobs() {
    struct Job* current = job_list;
    while (current != NULL) {
        printf("[%d]%d  %-20s %s\n", current->jobId, (int)current->pid,(current->status == RUNNING) 
        ? "Running" : (current->status == BLOCKED) ? "Blocked" : "Ready",current->command);
        current = current->next;
    }
}

// function to free the memory allocated for the doubly linked list
void freeJobList() {
    struct Job* current = job_list;
    while (current != NULL) {
        struct Job* temp = current;
        current = current->next;
        free(temp);
    }
}


// function to create a new node with given flag and pid
struct Revise* createNode(pid_t pid, int flag) {
    struct Revise* newNode = (struct Revise*)malloc(sizeof(struct Revise));
    if (newNode == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    newNode->pid = pid;
    newNode->flag = flag;
    newNode->next = NULL;
    return newNode;
}

// function to insert a node at the end of the linked list
void add(pid_t pid, int flag) {
    struct termios setting;
    if(tcgetattr(STDIN_FILENO, &setting)< 0) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }
    struct Revise* newNode = createNode(pid, flag);
    newNode ->setting = setting;
    if (revise_list == NULL) {
        revise_list = newNode;
    } else {
        struct Revise* temp = revise_list;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = newNode;
    }
}

// function to remove the first node from the linked list
struct Revise* pop() {
    if (revise_list == NULL) {
        return NULL;
    }
    struct Revise* temp = revise_list;
    revise_list = revise_list->next;
    return temp;
}

// function to print the linked list
void printList() {
    struct Revise* temp = revise_list;
    while (temp != NULL) {
        printf("PID: %d, Flag: %d\n", temp->pid, temp->flag);
        temp = temp->next;
    }
}

// function to free memory allocated for the linked list
void freeList() {
    struct Revise* temp;
    while (revise_list != NULL) {
        temp = revise_list;
        revise_list = revise_list->next;
        free(temp);
    }
}