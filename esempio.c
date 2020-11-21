#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>

#define NUM_MESS 0
#define SPAZIO_DISP 1
#define MUTEX_C 2
#define MUTEX_P 3
#define DIM 10
#define ESEC 20
#define NUM_PROD 20
#define NUM_CONS 10

struct prodcons
{
    int buffer[DIM];
    int testa;
    int coda;
};

void Wait_Sem(int id_sem, int semnum)
{
    struct sembuf sem_buf;

    sem_buf.sem_num = semnum;
    sem_buf.sem_op = -1;
    sem_buf.sem_flg = 0;

    semop(id_sem, &sem_buf, 1);
}

void Signal_Sem(int id_sem, int semnum)
{
    struct sembuf sem_buf;

    sem_buf.sem_num = semnum;
    sem_buf.sem_op = 1;
    sem_buf.sem_flg = 0;

    semop(id_sem, &sem_buf, 1);
}

void Produttore(struct prodcons *p, key_t id_sem)
{

    int valore_prodotto = rand() % 100;

    printf("Valore prodotto: %d, PID: %d\n", valore_prodotto, getpid());

    Wait_Sem(id_sem, SPAZIO_DISP);
    Wait_Sem(id_sem, MUTEX_P);

    p->buffer[p->testa] = valore_prodotto;
    p->testa = (++(p->testa)) % 10;

    Signal_Sem(id_sem, MUTEX_P);
    Signal_Sem(id_sem, NUM_MESS);
}

void Consumatore(struct prodcons *p, key_t id_sem)
{

    int valore_letto;

    Wait_Sem(id_sem, NUM_MESS);

    //printf("Errore qui?");

    Wait_Sem(id_sem, MUTEX_C);

    valore_letto = p->buffer[p->coda];

    p->coda = (++(p->coda)) % 10;

    printf("Valore letto: %d\n", valore_letto);

    Signal_Sem(id_sem, MUTEX_C);

    Signal_Sem(id_sem, SPAZIO_DISP);
}

void Pulisci(int id_shm, int id_sem)
{

    shmctl(id_shm, IPC_RMID, 0);
    semctl(id_sem, NUM_MESS, IPC_RMID);
    semctl(id_sem, SPAZIO_DISP, IPC_RMID);
    semctl(id_sem, MUTEX_P, IPC_RMID);
    semctl(id_sem, MUTEX_C, IPC_RMID);
}

int main()
{

    key_t shm_k = ftok("./esempio", 'a');
    key_t sem_k = ftok("./esempio", 'c');

    int id_shm = shmget(shm_k, sizeof(struct prodcons), IPC_CREAT | 0664);

    if (id_shm == 0)
    {
        perror("Errore nella shmget!!\n");
        exit(0);
    }

    int id_sem = semget(sem_k, 4, IPC_CREAT | 0664);

    if (id_sem == 0)
    {
        perror("Errore nella semget!!\n");
        exit(0);
    }

    semctl(id_sem, SPAZIO_DISP, SETVAL, DIM);
    semctl(id_sem, NUM_MESS, SETVAL, 0);
    semctl(id_sem, MUTEX_C, SETVAL, 1);
    semctl(id_sem, MUTEX_P, SETVAL, 1);

    struct prodcons *p;

    p = (struct prodcons *)shmat(id_shm, NULL, 0);

    p->testa = 0;
    p->coda = 0;

    for (int i = 0; i < NUM_CONS; i++)
    {
        int pid = fork();

        if (pid == 0) //figlio consumatore
        {
            printf("Sono il figlio consumatore, PID: %d\n", getpid());
                
                    Consumatore(p, id_sem);
                

            exit(0);
        }
    }

    for (int i = 0; i < NUM_PROD; i++)
    {
        int pid2 = fork();

        if (pid2 == 0) //figlio produttore
        {
            printf("Sono il figlio produttore, PID: %d\n", getpid());

            srand(getpid()*time(NULL));
            
                Produttore(p, id_sem);
            
            printf("Termino, PID: %d\n", getpid());
            exit(0);
        }
    }

    for (int i = 0; i < (NUM_CONS + NUM_PROD); i++)
    {
        wait(NULL);
    }

   Pulisci(id_shm, id_sem);

    return (0);
}