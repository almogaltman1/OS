#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> /*for PATH_MAX*/
#include <sys/types.h>
#include <dirent.h>
#include <threads.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <errno.h>


/*struct of queue node*/
union data /*only one field from the options*/
{
    char path[PATH_MAX]; /*for queueNode of dir*/
    long index_of_cv_arr; /*for queueNode of thread*/
};   
typedef struct queueNode
{
    union data data;
    struct queueNode *next;
} queueNode;

/*struct of the queue*/
typedef struct queue
{
    queueNode *first;
    queueNode *last;
    long len;
} queue;


/*global vairables*/
char *search_term = NULL;
atomic_int cnt_files = 0;
int num_threads = 0;
atomic_int num_threads_died = 0;
queue *dir_q = NULL;
queue *thread_q = NULL;
cnd_t *cv_arr = NULL;
atomic_int start_flag = 0;
atomic_int stop_flag = 0;
mtx_t q_lock;


/*helper functions*/

/*creates a new node with path and return pointer to it*/
queueNode *crate_path_node(char * path)
{
    queueNode *node = (queueNode *)malloc(sizeof(queueNode));
    union data data;
    strcpy(data.path, path);
    node->data = data;
    node->next = NULL;
    return node;
}

/*creates a new node with thread_index and return pointer to it*/
queueNode *crate_index_node(int index)
{
    queueNode *node = (queueNode *)malloc(sizeof(queueNode));
    union data data;
    data.index_of_cv_arr = index;
    node->data = data;
    node->next = NULL;
    return node;
}

/*creates an empty queue*/
queue *create_queue()
{
    queue *q = (queue *)malloc(sizeof(queue));
    q->first = NULL;
    q->last = NULL;
    q->len = 0;
    return q;
}

/*add node to the end of the queue*/
void add(queue *q, queueNode *new_node)
{
    q->len++;
    /*if q is empty, new node will be first and last*/
    if (q->first == NULL)
    {
        q->first = new_node;
        q->last = new_node;
    }
    /*if queue is not empty, add to end*/
    else
    {
        q->last->next = new_node;
        q->last = new_node;
    }
}

/*remove first node in queue and return a pointer to it*/
queueNode *remove_first(queue *q)
{
    queueNode *temp = q->first;

    if (q->first == NULL) /*queue is empty*/
    {
        return NULL;
    }

    q->len--;
    q->first = temp->next;
    if (q->first == NULL)
    {
        /*if now after the remove the queue is empty, need to also change last*/
        q->last = NULL;
    }

    return temp;

}

/*free queue*/
void free_queue(queue *q)
{
    queueNode *temp = NULL;
    while (q->first != NULL)
    {
        temp = q->first;
        q->first = temp->next;
        free(temp);
    }
    free(q);
}

int thread_search(void *i)
{
    long thread_index = (long)i;
    queueNode *curr_path_node = NULL, *new_path_node = NULL;
    queueNode *curr_thread_node = NULL, *new_thread_node = NULL;
    char curr_path[PATH_MAX];
    char new_file_or_dir_path[PATH_MAX];
    DIR *curr_dir = NULL, *check_open = NULL;
    struct dirent *de = NULL;
    struct stat curr_stat;
    int st = 1;

    while (start_flag == 0) { /*do not start until main thread say*/ }

    while (1)
    {
        mtx_lock(&q_lock);
        /*go to sleep if directory queue is empty. also, check if need to stop sreach*/
        while ((dir_q->first == NULL) && stop_flag == 0)
        {
            /*check if need to finish, if directory queue is empty and all other threads are sleeping*/
            if (dir_q->first == NULL && thread_q->len == num_threads - num_threads_died - 1)
            {
                stop_flag = 1;
                /*wake all threads*/
                for (int j = 0; j < num_threads; j++)
                {
                    /*we can call cnd_signal also to the current thread, nothing will happen*/
                    cnd_signal(&cv_arr[j]);
                }
                mtx_unlock(&q_lock);
            }
            else
            {
                /*add myself to sleeping threads queue, and go to sleep*/
                new_thread_node = crate_index_node(thread_index);
                add(thread_q, new_thread_node);
                cnd_wait(&cv_arr[thread_index], &q_lock);

                if (stop_flag == 1)
                {
                    mtx_unlock(&q_lock);
                    thrd_exit(0);
                }
                /*check if I am first thread, if not wake him and go back to sleep*/
                while (thread_q->first->data.index_of_cv_arr != thread_index)
                {
                    cnd_signal(&cv_arr[thread_q->first->data.index_of_cv_arr]);
                    cnd_wait(&cv_arr[thread_index], &q_lock);
                    if (stop_flag == 1)
                    {
                        mtx_unlock(&q_lock);
                        thrd_exit(0);
                    }
                }
                /*I am first, remove myself from queue*/
                curr_thread_node = remove_first(thread_q);
                free(curr_thread_node); /*we don't need the node anymore*/
            }
        }
        if (stop_flag == 1)
        {
            mtx_unlock(&q_lock);
            thrd_exit(0);
        }

        /*take first directory node for search*/        
        curr_path_node = remove_first(dir_q);
        mtx_unlock(&q_lock);
        /*take path from the directory node*/
        strcpy(curr_path, curr_path_node->data.path);
        free(curr_path_node); /*we don't need the node anymore*/


        /*do saerch*/
        curr_dir = opendir(curr_path); /*must work, we didn't enter pathes that can't be searched to the queue*/
        while ((de = readdir(curr_dir)) != NULL)
        {
            if ((strcmp(de->d_name, ".") != 0) && (strcmp(de->d_name, "..") != 0)) /*if . or .. we want to ignore*/
            {
                /*new_file_or_dir_path will be the path of curr dirent,
                so we want to add / to the end of curr_path and then concatinate the rellevant dirent name avery time*/
                strcpy(new_file_or_dir_path, curr_path);
                strcat(new_file_or_dir_path, "/");
                strcat(new_file_or_dir_path, de->d_name);

                /*get stat of this path*/
                st = stat(new_file_or_dir_path, &curr_stat);
                if (st != 0 && errno != 2) 
                {
                    num_threads_died++;
                    /*check if need to wake some thread instead*/
                    mtx_lock(&q_lock);
                    if (thread_q->first != NULL)
                    {
                        cnd_signal(&cv_arr[thread_q->first->data.index_of_cv_arr]);
                    }
                    mtx_unlock(&q_lock);
                    perror("stat failed in seraching");
                    thrd_exit(1);
                }
                /*check if dir, and if it is dir check if we need to add to the queue (if it serachable)*/
                if (st == 0 && S_ISDIR(curr_stat.st_mode))
                {
                    check_open = opendir(new_file_or_dir_path);
                    if (check_open != NULL)
                    {
                        closedir(check_open);
                        new_path_node = crate_path_node(new_file_or_dir_path);
                        mtx_lock(&q_lock);
                        add(dir_q, new_path_node);
                        /*wake next thread if exist*/
                        if (thread_q->first != NULL)
                        {
                            cnd_signal(&cv_arr[thread_q->first->data.index_of_cv_arr]);
                        }
                        mtx_unlock(&q_lock);

                    }
                    else
                    {
                        printf("Directory %s: Permission denied.\n", new_file_or_dir_path);
                    }
                }
                else
                {
                    /*check if file name contains the searech term*/
                    if (strstr(de->d_name, search_term) != NULL)
                    {
                        cnt_files++;
                        printf("%s\n" ,new_file_or_dir_path);
                    }
                }
            }
        }
        closedir(curr_dir);
    }
}

int main(int argc, char *argv[])
{
    char *root = NULL;
    DIR* dir = NULL;
    queueNode *root_node = NULL;
    int thread_res;
    int num_errors = 0;

    if (argc != 4) /*3 arguments + path of program*/
    {
        fprintf(stderr, "3 arguments are needed.\n");
        exit(1);
    }

    root = argv[1];
    search_term = argv[2]; /*search_term is global so all threads will have access to it*/
    num_threads = atoi(argv[3]); /*num_threads is global so all threads will have access to it*/
    thrd_t thread_ids[num_threads];

    /*check that root can be searched*/
    dir = opendir(root);
    if (dir == NULL)
    {
        perror("root directory specified in the arguments can't be searched");
        exit(1);
    }
    closedir(dir);

    /*create directories queue and enter root path.
    no need of lock, beacuse threads are not created yet*/
    dir_q = create_queue();
    root_node = crate_path_node(root);
    add(dir_q, root_node);
    /*create threads queue*/
    thread_q = create_queue();

    /*init cv and mutex*/
    mtx_init(&q_lock, mtx_plain);
    //mtx_init(&thread_q_lock, mtx_plain);
    cv_arr = (cnd_t *)calloc(num_threads, sizeof(cnd_t));
    for (int i = 0; i < num_threads; i++)
    {
        cnd_init(&cv_arr[i]);
    }

    /*create threads*/
    int rc;
    for (long j = 0; j < num_threads; j++)
    {
        rc = thrd_create(&thread_ids[j], thread_search, (void *)j);
        if (rc != thrd_success)
        {
            perror("failed to creat thread in main");
            exit(1);
        }
    }
    /*make start flag 1 so threads can start search*/
    start_flag = 1;

    /*wait for all thrreads to finish*/
    for (int i = 0; i < num_threads; i++)
    {
        thrd_join(thread_ids[i], &thread_res);
        if (thread_res == 1)
        {
            num_errors++;
        }
    }

    /*free memory*/
    mtx_destroy(&q_lock);
    for (int i = 0; i < num_threads; i++)
    {
        cnd_destroy(&cv_arr[i]);
    }
    free_queue(dir_q);
    free_queue(thread_q);
    free(cv_arr);

    if (num_errors == num_threads) /*all threads have died, just exit with no print*/
    {
        exit(1);
    }
    
    printf("Done searching, found %d files\n", cnt_files);
    if (num_errors > 0) /*some thread encounterd error, exit 1*/
    {
        exit(1);
    }
    /*else, all threads finished with no errors, exit 0*/
    exit(0);
}
