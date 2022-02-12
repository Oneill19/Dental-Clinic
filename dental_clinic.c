#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#define N 10
#define CLIENT 12
#define SOFA 4
#define HYGIENIST 3

typedef struct Node // structure for linked list
{
    struct Node *next;
    int value;
} Node;

typedef struct Queue // structure for queue data structure
{
    Node *head;
    Node *tail;
    int size;
} Queue;

// global variables
int count = 0;
sem_t mutex, clients, on_sofa, pay, get_pay, treat, clients_in_treat;
Queue *qc, *qs; // sofa queue and standing queue

void init_queue(Queue *q);      // initiallize queue
void enqueue(Queue *q, int n);  // enter node to the queue
int dequeque(Queue *q);         // delete node from the queue
void free_queue(Queue *q);      // free the queue
void *client_func(void *vn);    // function for clients threads
void *hygienist_func(void *vh); // function from hygienists threads

// main function
int main()
{
    // initiallize variables
    int i, ans_entery, ans_mutex, ans_client, ans_on_sofa, ans_pay, ans_get_pay, ans_treat, ans_clients_in_treat, cIndx[CLIENT], hIndx[HYGIENIST];
    pthread_t cArr[CLIENT], hArr[HYGIENIST];

    // initiallize semaphores
    ans_mutex = sem_init(&mutex, 0, 1);
    ans_client = sem_init(&clients, 0, N);
    ans_on_sofa = sem_init(&on_sofa, 0, SOFA);
    ans_pay = sem_init(&pay, 0, 0);
    ans_get_pay = sem_init(&get_pay, 0, 1);
    ans_treat = sem_init(&treat, 0, 0);
    ans_clients_in_treat = sem_init(&clients_in_treat, 0, HYGIENIST);
    if (ans_mutex == -1 || ans_client == -1 || ans_on_sofa == -1 || ans_pay == -1 || ans_get_pay == -1 || ans_treat == -1 || ans_clients_in_treat == -1)
    {
        fprintf(stderr, "Error creating semaphore!\n");
        exit(-1);
    }

    // initiallize queues
    qc = (Queue *)malloc(sizeof(Queue));
    if (qc == NULL)
    {
        fprintf(stderr, "Error allocating memory!\n");
        exit(-1);
    }
    qs = (Queue *)malloc(sizeof(Queue));
    if (qs == NULL)
    {
        fprintf(stderr, "Error allocating memory!\n");
        exit(-1);
    }
    init_queue(qc);
    init_queue(qs);

    // run the clients and hygienists threads
    for (i = 0; i < CLIENT; i++)
    {
        cIndx[i] = i + 1;
        int ans = pthread_create(&cArr[i], NULL, client_func, (void *)&cIndx[i]);
        if (ans != 0)
        {
            fprintf(stderr, "Error creating thread\n");
            exit(-1);
        }
    }
    for (i = 0; i < HYGIENIST; i++)
    {
        hIndx[i] = i + 1;
        int ans = pthread_create(&hArr[i], NULL, hygienist_func, (void *)&hIndx[i]);
        if (ans != 0)
        {
            fprintf(stderr, "Error creating thread\n");
            exit(-1);
        }
    }

    for (i = 0; i < CLIENT; i++)
        pthread_join(cArr[i], NULL);
    for (i = 0; i < HYGIENIST; i++)
        pthread_join(hArr[i], NULL);
    free_queue(qc);
    free_queue(qs);
    return 0;
}

// initiallize queue
void init_queue(Queue *q)
{
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

// enter node to the queue
void enqueue(Queue *q, int n)
{
    Node *nd = (Node *)malloc(sizeof(Node)); // allocate memory for the node
    if (nd == NULL)
    {
        fprintf(stderr, "Error allocating nodes memory!\n");
        exit(-1);
    }
    nd->value = n;
    nd->next = NULL;

    // insert node to the tail
    if (q->size == 0)
        q->head = nd;
    else
        q->tail->next = nd;
    q->tail = nd;
    q->size++;
}

// delete node from the queue
int dequeque(Queue *q)
{
    Node *n = q->head; // get the head of queue
    q->head = q->head->next;
    q->size--;
    int num = n->value;
    free(n);    // free the node
    return num; // return the value of the node
}

// free the queue
void free_queue(Queue *q)
{
    Node *temp;
    while (q->head != NULL)
    {
        temp = q->head;
        q->head = q->head->next;
        free(temp);
    }
    free(q);
}

// function for clients threads
void *client_func(void *vn)
{
    int n = *(int *)vn;
    while (1)
    {
        // enter to the clinic
        sem_wait(&mutex);
        if (count == N)
            printf("I'm Patient #%d, I'm out of clinic\n", n);
        sem_post(&mutex);
        sem_wait(&clients); // reduce the number of clients in the clinic
        sem_wait(&mutex);
        enqueue(qs, n); // enter the client to the standing queue
        count++;        // increase the number of clients in the clinic
        printf("I'm Patient #%d, I got into the clinic\n", n);
        sem_post(&mutex);

        sleep(1);

        // sit on sofa
        sem_wait(&on_sofa); // reduce one space on the sofa
        sem_wait(&mutex);
        n = dequeque(qs); // get the number of client who stand the most
        enqueue(qc, n);   // enter the client to the sofa queue
        printf("I'm Patient #%d, I'm sitting on the sofa\n", n);
        sem_post(&mutex);

        sleep(1);

        // get treat
        sem_wait(&clients_in_treat); // reduce the number of clients in treat
        sem_wait(&mutex);
        n = dequeque(qc); // get the number of client who seat the most on the sofa
        sem_post(&mutex);
        printf("I'm Patient #%d, I'm getting treatment\n", n);
        sem_post(&treat);   // let the hygienist know there is a client waiting
        sem_post(&on_sofa); // free one space on the sofa

        sleep(1);

        // pay on treat
        sem_wait(&get_pay); // only one payment at a time
        printf("I'm Patient #%d, I'm paying now\n", n);
        sem_post(&pay);

        sleep(1);
    }
}

// function from hygienists threads
void *hygienist_func(void *vh)
{
    int h = *(int *)vh;
    while (1)
    {   
        // give treat
        sem_wait(&treat); // wait for a client
        printf("I'm Dental Hygienist #%d, I'm working now\n", h);

        sleep(1);

        // get pay
        sem_wait(&pay); // get the payment after the client paid
        printf("I'm Dental Hygienist #%d, I'm getting a payment\n", h);
        sem_post(&get_pay); // free the payment space
        sem_wait(&mutex);
        count--; // decrease the number of clients in the clinic
        sem_post(&mutex);
        sem_post(&clients);          // free one space in the clinic
        sem_post(&clients_in_treat); // free one space on the treatments chair

        sleep(1);
    }
}
