#ifndef JOB_H
#define JOB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#define MAX_INPUT_SIZE 128

// all possible job statuses
enum JobStatus {
    RUNNING,
    BLOCKED,
    READY
};

// job struct using double linked list
struct Job {
    int jobId;
    int is_background;
    char command[MAX_INPUT_SIZE];
    enum JobStatus status;
    pid_t pid;
    struct termios setting;
    struct Job* next;
    struct Job* prev; 
};

struct Revise {
    pid_t pid;
    int flag;
    struct termios setting;
    struct Revise* next;
};

struct Job* job_list; // head of job LL
struct Job* tail;
int jobCounter; // total number of current jobs

// function to create a new job
struct Job* createJob(int is_background, const char* command, pid_t pid);

// function to add a job to the doubly linked list
void addJob(int is_background, const char* command, pid_t pid);

// function to remove a job from the doubly linked list
void removeJob(pid_t pid);

// function to print the doubly linked list
void printJobs();

// function to free the memory allocated for the doubly linked list
void freeJobList();

struct Revise* revise_list; // head of revise LL

// struct definition for the node in the linked list


// function to create a new node with given flag and pid
struct Revise* createNode(pid_t pid, int flag);

// function to insert a node at the end of the linked list
void add(pid_t pid, int flag);

// function to remove the first node from the linked list
struct Revise* pop();

// function to print the linked list
void printList();

// function to free memory allocated for the linked list
void freeList();

#endif /* JOB_H */
