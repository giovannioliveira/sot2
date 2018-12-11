#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

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
    int prev;
    int next;
} Page;

typedef struct {
    int id;
    int ws[WORKING_SET_LIMIT];
    int p;
    int pg[MAX_VIRTUAL_PAGES];
    int pgc;

    pthread_t thread;
} Process;

Process ps[MAX_PROCESS_IN_EXECUTION];
int psc = 0;

Page pg[MAX_PROCESS_IN_EXECUTION * MAX_PAGES_PER_PROCESS];
int pgc = 0;

Frame frames[MAIN_MEMORY_SIZE];
int head = -1;
int frc = 0;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;


void insertWs(int psid, int pgid){
    Process *psaux = &ps[psid];
    int i;
    for(i=0;i<WORKING_SET_LIMIT;i++){
        if(psaux->ws[i]==-1){
            psaux->ws[i]=pgid;
            return;
        }
    }
    assert(0);
}

void removeWs(int psid, int pgid){
    Process *psaux = &ps[psid];
    int i;
    for(i=0;i<WORKING_SET_LIMIT;i++){
        if(psaux->ws[i]==pgid){
            psaux->ws[i]=-1;
        }
    }
}

int getWsc(int psid){
    Process *psaux = &ps[psid];
    int i=0;
    while(i<WORKING_SET_LIMIT) {
        if(psaux->ws[i]!=-1)
            i++;
    }
    return i;
}

int contains(int *list, int len, int obj){
    puts("contains");
    int i;
    for(i=0;i<len;i++){
        if(list[i]==obj)
            return 1;
    }
    return 0;
}

int getLastPage(){
    puts("getLastPage");
    if(head==-1)
        return -1;
    int pgi = head;
    while(pg[pgi].next!=-1){
        pgi = pg[pgi].next;
    }
    printf("Page %d is the LRU\n",pgi);
    return pgi;
}

int getLRUPage(int psid){
    puts("getLRUPage");
    int i = getLastPage();
    if(psid==-1){
        return i;
    }
    while(!contains(ps[psid].ws,WORKING_SET_LIMIT,i)){
        i = pg[i].prev;
    }
    printf("Page %d is the LRU in process %d working set\n",i,psid);
    return i;
}

void bringToFront(int pgid){
    puts("bringToFront");
    Page *pgaux = &pg[pgid];
    Page *pgprev = pgaux->prev!=-1? &pg[pgaux->prev]: NULL;
    Page *pgnext = pgaux->next!=-1? &pg[pgaux->next]: NULL;

    if(pgprev==NULL) {
        return;
    }

    if(pgnext!=NULL){
        pgprev->next = pgnext->id;
        pgnext->prev = pgprev->id;
    }else{
        pgprev->next = -1;
    }

    pgaux->next = head;
    pg[head].prev = pgaux->id;
}

void printIntArray(int *arr, int len){
    puts("printIntArray");
    int i=0;
    for(i=0;i<len;i++){
        printf("%d ",arr[i]);
    }
}

int getEmptyFrame(){
    puts("getEmptyFrame");
    int i;
    for(i=0;i<MAIN_MEMORY_SIZE;i++){
        if(frames[i].page == -1){
            return i;
        }
    }
    return -1;
}

void swapInPage(int pgid){
    puts("swapInPage");
    Page *pgaux = &pg[pgid];
    Process *psaux = &ps[pgaux->psid];
    int aux = getEmptyFrame();
    assert(aux!=-1);

    Frame *fraux = &frames[aux];

    fraux->page=pgaux->id;
    frc++;

    pgaux->p=1;
    pgaux->fid=fraux->id;

    if(!contains(psaux->ws,WORKING_SET_LIMIT,pgid)){
        insertWs(psaux->id,pgid);
    }

    printf("Page %d swapped in\n",pgid);
}

void swapOutPage(int pgid){
    puts("swapOutPage");
    Page *pgaux = &pg[pgid];
    Frame *fraux = &frames[pgaux->fid];
    Process *psaux = &ps[pgaux->psid];

    fraux->page=-1;
    frc--;

    pgaux->p=0;
    pgaux->fid=-1;
    printf("Page %d swapped out\n",pgid);
}

void swapIn(int psid){
    puts("swapIn");
    Process *psaux = &ps[psid];
    int i;
    for(i=0;i<WORKING_SET_LIMIT;i++){
        if(psaux->ws[i]!=-1)
            swapInPage(psaux->ws[i]);
    }
    psaux->p=0;
    printf("Process %d swapped out with working set ",psid);
    printIntArray(&psaux->ws,WORKING_SET_LIMIT);
    printf("\n");
}

void swapOut(int psid){
    puts("swapOut");
    Process *psaux = &ps[psid];
    int i;
    for(i=0;i<WORKING_SET_LIMIT;i++){
        swapOutPage(psaux->ws[i]);
    }
    psaux->p=1;
    printf("Process %d swapped out with working set ",psid);
    printIntArray(psaux->ws,WORKING_SET_LIMIT);
    printf("\n");
    puts("swapOutOut");

}

void requestPage(int pgid) {
    puts("requestPage");
    pthread_mutex_lock(&mutex2);

    Page *pgaux = &pg[pgid];
    Process *psaux = &ps[pgaux->psid];
    printf("Process %d requested page %d\n",psaux->id,pgaux->id);

    if(!psaux->p){
        puts("a");
        while((MAIN_MEMORY_SIZE-frc)<getWsc(psaux->id)){
            int psid = pg[getLastPage()].psid;
            swapOut(psid);
        }
        puts("b");
        swapIn(psaux->id);
    }

    if(getWsc(psaux->id)==WORKING_SET_LIMIT){
        int lru = getLRUPage(psaux->id);
        swapOutPage(lru);
        removeWs(psaux->id,lru);
    }

    if(frc==MAIN_MEMORY_SIZE){
        swapOut(pg[getLastPage()].psid);
    }

    swapInPage(pgid);
    bringToFront(pgid);

    pthread_mutex_unlock(&mutex2);
}

void *processProcedure(void *arg) {
    puts("processProcedure");
    while (1) {
        int i = *((int*)arg);
        Process *psaux = &(ps[i]);
        int pgid = psaux->pg[rand() % psaux->pgc];
        requestPage(pgid);
        sleep(PROCESS_REQUEST_PAGE_INTERVAL);
    }
}


void createProcess() {
    puts("createProcess");
    pthread_t *psthread;
    pthread_mutex_lock(&mutex1);
    Process *psaux = &ps[psc];
    psaux->id = psc;
    psaux->pgc = ((rand() % MAX_PAGES_PER_PROCESS) + 1);

    int i;
    for (i = 0; i < psaux->pgc; i++) {
        Page *pgaux = &pg[pgc];
        pgaux->id = pgc;
        pgaux->psid = psaux->id;
        pgaux->p = 0;
        pgaux->fid = -1;
        pgaux->prev = -1;
        pgaux->next = -1;
        pgc++;

        psaux->pg[i] = pgaux->id;
    }

    for(i=0;i<WORKING_SET_LIMIT;i++){
        psaux->ws[i] = -1;
    }

    psaux->p = 0;
    psc++;

    psthread = &(psaux->thread);
    printf("Process %d created with pages from %d to %d\n",psaux->id,psaux->pg[0],psaux->pg[psaux->pgc-1]);
    pthread_mutex_unlock(&mutex1);

    pthread_create(psthread, NULL, processProcedure, &psaux->id);

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