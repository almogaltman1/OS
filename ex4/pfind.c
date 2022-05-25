#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> /*for PATH_MAX*/
#include <sys/types.h>
#include <dirent.h>
#include <threads.h>
#include <sys/stat.h>


/*struct of queue node*/
union data /*only one field from the options*/
{
    char path[PATH_MAX]; /*for queueNode of dir*/
    int index_of_cv_arr; /*for queueNode of thread*/
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
int cnt_files = 0;
int num_threads = 0;
queue *dir_q = NULL;


/*helper functions*/

/*creates a new node with path and return pointer to it*/
queueNode *crate_node(union data *data)
{
    queueNode *node = (queueNode *)malloc(sizeof(queueNode)); /*need to check????????????????*/
    node->data = *data;
    node->next = NULL;
    return node;
}

/*creates an empty queue*/
queue *create_queue()
{
    queue *q = (queue *)malloc(sizeof(queue)); /*need to check????????????????*/
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
    if (q->last == NULL)
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


int thread_search(void *q)
{
    queue *dir_q = (queue *)q;
    queueNode *curr_path_node = NULL, *new_path_node = NULL;
    char curr_path[PATH_MAX];
    char new_file_or_dir_path[PATH_MAX];
    DIR *curr_dir = NULL;
    struct dirent *de = NULL;
    struct stat curr_stat;
    union data data_node;

    while (dir_q->first != NULL)
    {
    /*need to wait for all to start!!!!!!!!!!!!!!!!!!!!!!!*/
    //printf("im here 1\n"); /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

    /*check if queue is empty, if wmpty go to sleep*/
    while (dir_q->first == NULL)
    {
        //sleep!!!!!!!!!!!!!!!!!!!!!!! note - like this will sleep only once, need to do somethong else
    }
    /*take first directory from queue*/
    curr_path_node = remove_first(dir_q);
    stpcpy(curr_path, curr_path_node->data.path);
    printf("curr path is %s\n", curr_path);
    free(curr_path_node); /*we don't need the node anymore*/

   
    
    /*do saerch*/
    curr_dir = opendir(curr_path); /*must work, we didn't enter pathes that can't be searched to the queue*/
    while ((de = readdir(curr_dir)) != NULL)
    {
        if ((strcmp(de->d_name, ".") != 0) && (strcmp(de->d_name, "..") != 0)) /*if . or .. we want to ignore*/
        {
            //printf("im here 2\n"); /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
            /*new_file_or_dir_path will be the path of curr dirent,
            so we want to add / to the end of curr_path and then concatinate the rellevant dirent name avery time*/
            strcpy(new_file_or_dir_path, curr_path);
            strcat(new_file_or_dir_path, "/");
            strcat(new_file_or_dir_path, de->d_name);

            /*get stat of this path*/
            //printf("im here before stat\n"); /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
            if (stat(new_file_or_dir_path, &curr_stat) != 0)
            {
                perror("stat failed in seraching\n");
                thrd_exit(1);
            }
            //printf("im here after stat\n"); /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
            /*check if dir, and if it is dir if we need to add to the queue (if it serachable)*/
            if (S_ISDIR(curr_stat.st_mode))
            {
                //printf("im here 3\n"); /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
                if (opendir(new_file_or_dir_path) != NULL)
                {
                    strcpy(data_node.path,new_file_or_dir_path);
                    new_path_node = crate_node(&data_node);
                    add(dir_q, new_path_node);
                }
                else
                {
                    printf("Directory %s: Permission denied.\n", new_file_or_dir_path);
                }
                //printf("im here 4\n"); /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
            }
            else /*it is a file??????????????????????*/
            {
                //printf("im here 5\n"); /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
                /*check if file name contains the searech term*/
               //sub_string = de->d_name;
                //printf("de is %s\n", de->d_name); /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
                //printf("st is %s\n", search_term);
                if (strstr(de->d_name, search_term) != NULL)
                {
                    cnt_files++;
                    printf("%s\n", new_file_or_dir_path);
                }
                //printf("im here 6\n"); /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
            }
            //printf("im here 7\n"); /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
            printf("de is %s\n", de->d_name); /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
            //printf("im here 8\n"); /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
        }
    }
    closedir(curr_dir);
    }
    thrd_exit(0);
    
}

int main(int argc, char *argv[])
{
    char *root = NULL;
    DIR* dir = NULL;
    queueNode *root_node = NULL;
    union data root_data;
    int thread_res;
    int num_errors = 0;

    if (argc != 4) /*3 arguments + path of program*/
    {
        fprintf(stderr, "3 arguments are needed.\n"); /*maybe change??????????????????????*/
        exit(1);
    }

    root = argv[1];
    search_term = argv[2]; /*search_term is global so all threads will have access to it*/
    num_threads = atoi(argv[3]);
    thrd_t thread_ids[num_threads]; /*probably will change!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

    /*check that root can be searched*/
    dir = opendir(root);
    if (dir == NULL)
    {
        perror("root directory specified in the arguments can't be searched\n");
        exit(1);
    }
    closedir(dir);

    /*create directories queue and enter root path*/
    dir_q = create_queue(); /*need to check????????????????*/
    strcpy(root_data.path, root);
    root_node = crate_node(&root_data); /*need to check????????????????*/
    add(dir_q, root_node);

    /*create threads*/
    /*will change when add multiple threads!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    int rc;
    for (int i = 0; i < num_threads; i++)
    {
        rc = thrd_create(&thread_ids[i], thread_search, (void *)dir_q);
        if (rc != thrd_success)
        {
            perror("failed to creat thread in main\n");
            exit(1);
        }
    }

    for (int i = 0; i < num_threads; i++)
    {
        thrd_join(thread_ids[i], &thread_res);
        if (thread_res == 1)
        {
            num_errors++;
        }
    }

    if (num_errors == num_threads) /*all threads have died, just exit with no print*/
    {
        exit(1);
    }
    
    printf("Done searching, found %d files\n", cnt_files);
    if (num_errors > 0) /*some thread encounterd error, exit 1*/
    {
        exit(1);
    }

    /*free memory!!!!!!!!!!!!!!!!! free queues, cv array,*/
    /*else, all threads finished with no errors, exit 0*/
    printf("done\n");
    exit(0);
}
