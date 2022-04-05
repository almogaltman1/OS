#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>


int regular_process(char **arglist);
int background_process(int count, char **arglist);
int pipe_process(char **arglist, int pipe_index);
int pipe_index(int count, char **arglist);
int redirection_process(int count, char **arglist);


/*helper functions*/
/*This function is for regular process (without & or | or >>).
Parent waits for his child, so no other commands will be accepted before child completes*/
int regular_process(char **arglist)
{
    pid_t pid = fork();
    if (pid == -1)
    {
        /*if fork fails, it is an error in the parent process and we want to return 0*/
        perror("Failed forking");
        return 0;
    }
    else if (pid == 0)
    {
        /*child process*/
        /*if an error occurs in a child process we want to print an error message,
        and terminate only the child process using exit(1).*/

        if (signal(SIGINT, SIG_DFL) == SIG_ERR) /*make child process to default handle of SIGINT*/
        {
            /*is this ok????????????????*/
            perror("Signal failed in child process");
            exit(1);
        }
        if (execvp(arglist[0], arglist) == -1)
        {
            perror("Failed creating process");
            exit(1);
        }
    }
    else
    {
        /*parent process*/
        waitpid(pid, NULL, 0);
    }
    return 1;
}

/*This function is for background process (with &).
So parent is not waiting for his child and other commands can run*/
int background_process(int count, char **arglist)
{
    arglist[count - 1] = NULL; /*turns the & to NULL.*/
    pid_t pid = fork();
    if (pid == -1)
    {
        /*if fork fails, it is an error in the parent process and we want to return 0*/
        perror("Failed forking");
        return 0;
    }
    else if (pid == 0)
    {
        /*child process*/
        if (execvp(arglist[0], arglist) == -1)
        {
            /*if an error occurs in a child process we want to print an error message,
            and terminate only the child process using exit(1).*/
            perror("Failed creating process");
            exit(1);
        }
    }
    return 1;
}

/*This function is for piping process (with |).
Parent waits for both childs, so no other commands will be accepted before childs complete*/
int pipe_process(char **arglist, int pipe_index)
{
    char **second_arglist = arglist + pipe_index + 1; /*ponits to the start of second command*/
    arglist[pipe_index] = NULL; /*turns the | to NULL.*/
    int pipefd[2];
    pid_t pid, pid2;

    if (pipe(pipefd) == -1)
    {
        /*if pipe fails, it is an error in the parent process and we want to return 0*/
        perror("Failed pipe");
        return 0;
    }

    pid = fork();
    if (pid == -1)
    {
        /*if fork fails, it is an error in the parent process and we want to return 0*/
        perror("Failed forking");
        return 0;
    }
    else if (pid == 0)
    {
        /*first child process - will be the writer*/
        /*if an error occurs in a child process we want to print an error message,
        and terminate only the child process using exit(1).*/

        if (signal(SIGINT, SIG_DFL) == SIG_ERR) /*make child process to default handle of SIGINT*/
        {
            /*is this ok????????????????*/
            perror("Signal failed in child process");
            exit(1);
        }
        if (close(pipefd[0]) == -1)
        {
            perror("Failed close");
            exit(1);
        }
        if (dup2(pipefd[1] , 1) == -1) /*redirecting stdout to the pipe write side*/
        {
            perror("Failed dup");
            exit(1);
        } 
        if (close(pipefd[1]) == -1)
        {
            perror("Failed close");
            exit(1);
        }
        if (execvp(arglist[0], arglist) == -1)
        {
            perror("Failed creating process");
            exit(1);
        }
    }
    else
    {
        /*parent process, will make another child*/
        pid2 = fork();
        if (pid2 == -1)
        {
            /*if fork fails, it is an error in the parent process and we want to return 0*/
            perror("Failed forking");
            return 0;
        }
        else if (pid2 == 0)
        {
            /*second child process - will be the reader*/
            /*if an error occurs in a child process we want to print an error message,
            and terminate only the child process using exit(1).*/

            if (signal(SIGINT, SIG_DFL) == SIG_ERR) /*make child process to default handle of SIGINT*/
            {
                /*is this ok????????????????*/
                perror("Signal failed in child process");
                exit(1);
            }
            if (close(pipefd[1]) == -1)
            {
                perror("Failed close");
                exit(1);
            }
            if (dup2(pipefd[0] , 0) == -1) /*redirecting stdin to the pipe read side*/
            {
                perror("Failed dup");
                exit(1);
            }
            if (close(pipefd[0]) == -1)
            {
                perror("Failed close");
                exit(1);
            }
            if (execvp(second_arglist[0], second_arglist) == -1)
            {
                perror("Failed creating process");
                exit(1);
            }
        }
        else
        {
            /*parent process*/
            /*if close fails here, it is an error in the parent process and we want to return 0*/
            if (close(pipefd[0]) == -1)
            {
                perror("Failed close in parent");
                return 0;
            }
            if (close(pipefd[1]) == -1)
            {
                perror("Failed close in parent");
                return 0;
            }
            waitpid(pid, NULL, 0);
            waitpid(pid2,NULL,0);
        }   
    }
    return 1;
    
}

/*returns the | symbol's index if exist in arglist, otherwise returns -1.*/
int pipe_index(int count, char **arglist)
{
    for(int i = 0; i < count; i++)
    {
        if (strcmp(arglist[i], "|") == 0)
        {
            return i;
        }
    }

    return -1; /*sign | is not found*/
}

/*This function is for redirecting process (with >>).
Parent waits for his child, so no other commands will be accepted before child completes*/
int redirection_process(int count, char **arglist)
{
    char *file_name = arglist[count - 1];
    arglist[count - 2] = NULL; /*turns the >> to NULL.*/
    int fd;
    pid_t pid;

    pid = fork();
    if (pid == -1)
    {
        /*if fork fails, it is an error in the parent process and we want to return 0*/
        perror("Failed forking");
        return 0;
    }
    else if (pid == 0)
    {
        /*child process*/
        /*if an error occurs in a child process we want to print an error message,
        and terminate only the child process using exit(1).*/

        if (signal(SIGINT, SIG_DFL) == -1) /*make child process to default handle of SIGINT*/
        {
            /*is this ok????????????????*/
            perror("Signal failed in child process");
            exit(1);
        }
        fd = open(file_name, O_CREAT | O_APPEND | O_RDWR, 666);
        if (fd == -1)
        {
            perror("Failed opening file");
            exit(1);
        }
        if (dup2(fd , 1) == -1) /*redirecting stdout to the file*/
        {
            perror("Failed dup");
            exit(1);
        }
        if (close(fd) == -1)
        {
            perror("Failed close");
            exit(1);
        }
        if (execvp(arglist[0], arglist) == -1)
        {
            perror("Failed creating process");
            exit(1);
        }
    }
    else
    {
        /*parent process*/
        waitpid(pid, NULL, 0);
    }
    return 1;
}



/*functions from the assignment*/
int prepare(void)
{
    /*if an error occurs in a signal handler in the shell parent process,
    we want to print an error message, and terminate the shell process with exit(1)*/

    if (signal(SIGINT, SIG_IGN) == SIG_ERR) /*make process ignore SIGINT*/
    {
        perror("Error in signal handler");
        exit(1);
    }
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) /*handling zombies??????????????????????????????*/
    {
        perror("Error in signal handler");
        exit(1); 
    }
    return 0;
}

/*should return 1 if no error occurs and 0 otherwise*/
int process_arglist(int count, char **arglist)
{
    int pipe_ind = -1; /*this will be the index of the | if it exist*/
    /*checks background*/
    if (strcmp(arglist[count - 1], "&") == 0)
    {
        return background_process(count, arglist);
    }
    /*checks redirection*/
    if (count > 1 && strcmp(arglist[count - 2], ">>") == 0)
    {
        return redirection_process(count,arglist);
    }
    /*checks pipe*/
    pipe_ind = pipe_index(count, arglist);
    if (pipe_ind != -1)
        {
            return pipe_process(arglist, pipe_ind);
        }

    /*else, regular*/
    return regular_process(arglist);
}

int finalize(void)
{
    return 0;
}