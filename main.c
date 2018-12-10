#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>

#define MAIN_MEMORY_SIZE 64
#define MAX_VIRTUAL_PAGES 64
#define PROCESS_CREATION_INTERVAL 3
#define PROCESS_REQUEST_PAGE_INTERVAL 3
#define WORKING_SET_LIMIT 4
#define MAX_PAGES_PER_PROCESS 64
#define MAX_PROCESS_IN_EXECUTION 10

//estratégia de realocação: LRU
//swap out completo -> realocação global
//swap in completo
//monitoramento e logs

typedef struct {
    int id;
    int page;
} Frame;

typedef struct {
    int id;
    int psid;
    int p;
    int fid;
} Page;

typedef struct {
    int id;
    int ws[WORKING_SET_LIMIT];
    int wsc ;
    int head;
    int tail;
    int suspended;
    int pg[MAX_VIRTUAL_PAGES];
    int pgc;

    pthread_t thread;
} Process;

Process ps[MAX_PROCESS_IN_EXECUTION];
int psc = 0;

Page pg[MAX_PROCESS_IN_EXECUTION * MAX_PAGES_PER_PROCESS];
int pgc = 0;

Frame frames[MAIN_MEMORY_SIZE];
int head = -1, tail = -1;
int frc = 0;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

//int getEmptyFrame(){
//    int i;
//    for(i=0;i<MAIN_MEMORY_SIZE;i++){
//        if(frames[i].page == -1){
//            return i;
//        }
//    }
//    return -1;
//}

void requestPage(int pgid) {
    pthread_mutex_lock(&mutex2);

    Page *pgaux = &pg[pgid];
    Process *psaux = &ps[pgaux->psid];
    printf("Process %d requested page %d\n",psaux->id,pgaux->id);

    //update LRU
    pthread_mutex_unlock(&mutex2);
}

void *processProcedure(void *arg) {
    while (1) {
        int i = *((int*)arg);
        Process *psaux = &(ps[i]);
        int pgid = psaux->pg[rand() % psaux->pgc];
        requestPage(pgid);
        sleep(PROCESS_REQUEST_PAGE_INTERVAL);
    }
}


void createProcess() {
    pthread_t *psthread;
    pthread_mutex_lock(&mutex1);
    Process *psaux = &ps[psc];
    psaux->id = psc;
    psaux->pgc = ((rand() % MAX_PAGES_PER_PROCESS) + 1);
    printf("%d\n",psaux->pgc);
    int i;
    for (i = 0; i < psaux->pgc; i++) {
        Page *pgaux = &pg[pgc];
        pgaux->id = pgc;
        pgaux->psid = psaux->id;
        pgaux->p = 0;
        pgaux->fid = -1;
        pgc++;

        psaux->pg[i] = pgaux->id;
    }

    psaux->wsc = 0;
    psaux->head = -1;
    psaux->tail = -1;
    psaux->suspended = 1;
    psc++;
    printf("%d\n",psaux->pgc);
    psthread = &(psaux->thread);
    pthread_mutex_unlock(&mutex1);

    //printf("%d\n",psaux->pgc);

    pthread_create(psthread, NULL, processProcedure, &psaux->id);

    printf("Process %d created with pages from %d to %d\n",psaux->id,psaux->pg[0],psaux->pg[psaux->pgc-1]);

}

void initFrames(){
    int i;
    for(i=0;i<MAIN_MEMORY_SIZE;i++){
        Frame *f = &(frames[i]);
        f->id = i;
        f->page=-1;
    }
}


int main() {

    initFrames();
    srand(time(NULL));
    int i = 0;
    while (i<MAX_PROCESS_IN_EXECUTION) {
        createProcess();
        i++;
        sleep(PROCESS_CREATION_INTERVAL);
    }
    getc(stdin);

}