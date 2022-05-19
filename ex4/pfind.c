#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> /*for PATH_MAX*/
#include <sys/types.h>
#include <dirent.h>


/*struct of queue node*/
typedef struct queueNode
{
    char path[PATH_MAX];
    struct queueNode *next;
} queueNode;

/*struct of the queue*/
typedef struct queue
{
    queueNode *first;
    queueNode *last;
} queue;


/*helper functions*/

/*creates a new node with path and return pointer to it*/
queueNode *crate_node(char *path)
{
    queueNode *node = (queueNode *)malloc(sizeof(queueNode));
    strcpy(node->path, path);
    node->next = NULL;
    return node;
}

/*creates an empty queue*/
queue *create_queue()
{
    queue *q = (queue *)malloc(sizeof(queue));
    q->first = NULL;
    q->last = NULL;
    return q;
}

/*add node to the end of the queue*/
void add(queue *q, queueNode *new_node)
{
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

    q->first = temp->next;
    if (q->first == NULL)
    {
        /*if now after the remove the queue is empty, need to also change last*/
        q->last = NULL;
    }

    return temp;

}

int main(int argc, char *argv[])
{
    char *root = NULL;
    char *search_term = NULL;
    int num_threads = 0;
    DIR* dir = NULL;

    if (argc != 4) /*3 arguments + path of program*/
    {
        fprintf(stderr, "3 arguments are needed.\n"); /*maybe change??????????????????????*/
        exit(1);
    }

    root = argv[1];
    search_term = argv[2];
    num_threads = atoi(argv[3]);

    /*check that root can be searched*/
    dir = opendir(root);
    if (dir == NULL)
    {
        perror("root directory specified in the arguments can't be searched\n");
        exit(1);
    }

    return 0; /*?????????????????*/
}
