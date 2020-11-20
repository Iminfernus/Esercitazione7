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

#define NUM_MESS 0
#define SPAZIO_DISP 1
#define DIM 10
#define ESEC 30


struct prodcons
{
    int buffer [DIM];
    int testa;
    int coda;

};


void Wait_Sem(int id_sem, int semnum)
{
    struct sembuf sem_buf;

    sem_buf.sem_num = semnum;
    sem_buf.sem_op = -1;
    sem_buf.sem_flg = 0;

    semop(id_sem, &sem_buf,1);

    
}

void Signal_Sem(int id_sem, int semnum)
{
    struct sembuf sem_buf;
    
    sem_buf.sem_num = semnum;
    sem_buf.sem_op = 1;
    sem_buf.sem_flg = 0;

    semop(id_sem, &sem_buf, 1);
    
}

void Produttore(struct prodcons *p, key_t id_sem){

    int valore_prodotto = rand() % 10;

    printf("Valore prodotto: %d\n", valore_prodotto);

    Wait_Sem(id_sem, SPAZIO_DISP);

    p->buffer[p->testa] = valore_prodotto;
    p->testa = (++(p->testa)) % 10;


    Signal_Sem(id_sem,NUM_MESS);

}

void Consumatore(struct prodcons *p, key_t id_sem){

    int valore_letto;

    Wait_Sem(id_sem,NUM_MESS);

    valore_letto = p->buffer[p->coda];

    p->coda = (++(p->coda)) % 10;

    printf("Valore letto: %d\n", valore_letto);


    Signal_Sem(id_sem,SPAZIO_DISP);

}
 
void Pulisci(key_t shm_k, key_t sem_k) {

    shmctl(shm_k, IPC_RMID, NULL);
    semctl(sem_k,NUM_MESS,IPC_RMID);
    semctl(sem_k,SPAZIO_DISP,IPC_RMID);  

}

int main() {

    key_t shm_k = ftok("./esempio", 'a');
    key_t sem_k = ftok("./esempio", 'c');

    int id_shm = shmget(shm_k, sizeof(struct prodcons), IPC_CREAT | 0664);

    if (id_shm == 0)
    {
        perror("Errore nella shmget!!\n");
        exit(0);
    }

    int id_sem = semget(sem_k, 2, IPC_CREAT | 0664);

    if (id_sem == 0)
    {
        perror("Errore nella semget!!\n");
        exit(0);
    }

    semctl(id_sem, SPAZIO_DISP, SETVAL ,DIM);
    semctl(id_sem, NUM_MESS,SETVAL, 0);

    struct prodcons * p;

    p = (struct prodcons *) shmat(id_shm, NULL, 0);

    p->testa = 0;
    p->coda = 0;


    int pid = fork();

    if (pid == 0)      //figlio consumatore
    {
        printf("Sono il figlio consumatore, PID: %d\n", getpid());

        for (int i = 0; i < ESEC; i++)
        {
            Consumatore(p,id_sem);
        }
        
        

        exit(0);
    }
    
    int pid2 = fork();

    if (pid2 == 0)     //figlio produttore
    {
        printf("Sono il figlio produttore, PID: %d\n", getpid());

        for (int i = 0; i < ESEC; i++)
        {
            Produttore(p,id_sem);
        
        }
        
        exit(0);
    }


    wait(NULL);
    wait(NULL);
    
    Pulisci(shm_k,sem_k);

    return(0); 

}