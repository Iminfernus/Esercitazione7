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

#define SYNCH 0
#define MUTEXL 1
#define MUTEXS 2
#define MUTEX 3

#define DIM 3
#define NUM_SCRIT 5
#define NUM_LETT 25


struct lettscrit
{
    int buffer;
    int num_lettori;
    int num_scrittori;

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

void Inizio_Lettura(int id_sem, struct lettscrit *p){

    Wait_Sem(id_sem, MUTEXL);

    p->num_lettori = (p->num_lettori) + 1;
    
    if ((p->num_lettori) == 1)
    {
        Wait_Sem(id_sem,SYNCH);
    }
    Signal_Sem(id_sem,MUTEXL);

}

void Fine_Lettura(int id_sem, struct lettscrit *p){
    Wait_Sem(id_sem, MUTEXL);
    p->num_lettori= (p->num_lettori) - 1;
    if ((p->num_lettori) == 0)
    {
        Signal_Sem(id_sem,SYNCH);
    }
    Signal_Sem(id_sem,MUTEXL);

}

void Inizio_Scrittura(int id_sem, struct lettscrit *p){
    Wait_Sem(id_sem, MUTEXS);
    p->num_scrittori++;
    if (p->num_scrittori == 1)
    {
        Wait_Sem(id_sem,SYNCH);
    }
    Signal_Sem(id_sem, MUTEXS);

    Wait_Sem(id_sem, MUTEX);
}

void Fine_Scrittura(int id_sem, struct lettscrit *p){
    
    Signal_Sem(id_sem, MUTEX);

    Wait_Sem(id_sem, MUTEXS);
    p->num_scrittori--;
    if (p->num_scrittori == 0)
    {
        Signal_Sem(id_sem, SYNCH);
    }
    Signal_Sem(id_sem, MUTEXS);

}

void Pulisci(int id_shm, int id_sem)
{
    shmctl(id_shm, IPC_RMID, 0);
    semctl(id_sem, MUTEXL, IPC_RMID);
    semctl(id_sem, SYNCH, IPC_RMID);
    semctl(id_sem, MUTEXS, IPC_RMID);
    semctl(id_sem, MUTEX, IPC_RMID);
}

int main()
{

    key_t shm_k = ftok("./esempio_let-scrit", 'a');
    key_t sem_k = ftok("./esempio_let-scrit", 'b');

    int id_shm = shmget(shm_k, sizeof(struct lettscrit), IPC_CREAT | 0664);

    if (id_shm == 0)
    {
        perror("Errore nella shmget!!\n");
        exit(1);
    }

    int id_sem = semget(sem_k, 4, IPC_CREAT | 0664);

    if (id_sem == 0)
    {
        perror("Errore nella semget!!\n");
        exit(1);
    }

    semctl(id_sem, SYNCH, SETVAL, 1);
    semctl(id_sem, MUTEXL, SETVAL, 1);
    semctl(id_sem, MUTEXS, SETVAL, 1);
    semctl(id_sem, MUTEX, SETVAL, 1);

    struct lettscrit *p;

    p = (struct lettscrit *)shmat(id_shm, NULL, 0);

    p->buffer = 0;
    p->num_lettori = 0;


    for (int i = 0; i < NUM_LETT; i++)
    {
        int pid = fork();

        if (pid == 0) //figlio lettore
        {
            //printf("[%d] Sono il figlio LETTORE. \n", getpid());

            Inizio_Lettura(id_sem,p);

            printf("[%d] Leggo: %d\n", getpid(), p->buffer);    
            
            Fine_Lettura(id_sem,p);
            
            printf("[%d] Termino. \n", getpid());

            exit(0);
        }
    }

    for (int i = 0; i < NUM_SCRIT; i++)
    {
        int pid2 = fork();

        if (pid2 == 0) //figlio scrittore
        {
            //printf("[%d] Sono il figlio SCRITTORE. \n", getpid());

            srand(getpid()*time(NULL));
            
            Inizio_Scrittura(id_sem,p);

            p->buffer = rand() % 100;

            printf("[%d] Scrivo: %d\n", getpid(), p->buffer);


            Fine_Scrittura(id_sem, p);
            
            printf("[%d] Termino.\n", getpid());
            
            exit(0);
        }
    }

    for (int i = 0; i < (NUM_LETT + NUM_SCRIT); i++)
    {
        wait(NULL);
    }

   Pulisci(id_shm, id_sem);

    return (0);
}