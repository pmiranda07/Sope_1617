#include "request.h"

FILE* WFile;
int capacidade;
int rec_F, rec_M, rej_F, rej_M, done_F, done_M;
char genderUsing='N';
struct timeval t0;
int faltamLer;
int ENT_FIFO_FD;
int REJ_FIFO_FD;
int ocupantes=0;
pthread_t threadsTid[255];
int threadPos=0;

pthread_mutex_t threadMutex = PTHREAD_MUTEX_INITIALIZER;


void printStats(){

    printf(" -- SAUNA RECEBIDOS -- \n");
    printf(" > M: %d\n", rec_M);
    printf(" > F: %d\n", rec_F);
    printf(" > TOTAL: %d\n\n", rec_F+rec_M);

    printf(" -- SAUNA SERVIDOS -- \n");
    printf(" > M: %d\n", done_M);
    printf(" > F: %d\n", done_F);
    printf(" > TOTAL: %d\n\n", done_F+done_M);

    printf(" -- SAUNA REJEITADOS -- \n");
    printf(" > M: %d\n", rej_M);
    printf(" > F: %d\n", rej_F);
    printf(" > TOTAL: %d\n", rej_F+rej_M);

}

void writeToFile(Request *req, int tid, char* tip){

  struct timeval t1;
  gettimeofday(&t1, NULL);
  double dt = (t1.tv_sec - t0.tv_sec)*1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;

  fprintf(WFile, "%-9.2f - %-4d - %-12d - %-4d: %-1c - %-4d - %-10s\n", dt, getpid(), tid ,req->id,req->gender, req->duration, tip);

  if(req->gender=='M'){
    if(strcmp(tip,"REJEITADO")==0) rej_M++;
    if(strcmp(tip,"RECEBIDO")==0) rec_M++;
    if(strcmp(tip,"SERVIDO")==0) done_M++;
  }
  else{
    if(strcmp(tip,"REJEITADO")==0) rej_F++;
    if(strcmp(tip,"RECEBIDO")==0) rec_F++;
    if(strcmp(tip,"SERVIDO")==0) done_F++;
  }

}

void DealReject(Request* req){

    req->denials = req->denials + 1;

    if(req->denials < 3)
      faltamLer++;

    write(REJ_FIFO_FD, req, sizeof(Request));
}

void *RequestStays(void *arg) {

  Request *req = (Request*)arg;


    usleep(req->duration*100);

    writeToFile(req, (int)pthread_self(),"SERVIDO");

    pthread_mutex_lock(&threadMutex);
    ocupantes--;
    pthread_mutex_unlock(&threadMutex);


    if(ocupantes==0){
      genderUsing = 'N';
    }

    pthread_exit(NULL);
}


void DealRequest(Request* req){


            if(genderUsing=='N') {
                      genderUsing = req->gender;
                      ocupantes++;
                      writeToFile(req, getpid(),"RECEBIDO");
                      pthread_create(&threadsTid[threadPos], NULL,RequestStays,req);
                      threadPos++;
            } else {
                      if(req->gender == genderUsing && ocupantes < capacidade) {
                          ocupantes++;
                          writeToFile(req, getpid(),"RECEBIDO");
                          pthread_create(&threadsTid[threadPos], NULL, RequestStays,req);
                          threadPos++;
                      } else {
                          writeToFile(req, getpid(),"RECEBIDO");
                          writeToFile(req, getpid(), "REJEITADO");
                          DealReject(req);

                      }
              }
  return;
}

void Receptor() {
        Request* req;
        int i;
        while(faltamLer != 0) {
                req = malloc(sizeof(Request));
                i=read(ENT_FIFO_FD, req, sizeof(Request));
                if(i>0){
                  DealRequest(req);
                  faltamLer--;
                }
        }
        if(faltamLer==0){
          Request* req =malloc(sizeof(Request));
          req->id=-1;
          write(REJ_FIFO_FD,  req, sizeof(Request));
          close(REJ_FIFO_FD);
          close(ENT_FIFO_FD);
        }
        return;
}

void openFifo() {

	while ((ENT_FIFO_FD = open(gen_fifo, O_RDONLY)) == -1) {
		if (errno == EEXIST)
			printf("Sauna info: Erro a ler fifo de entrada. Tentando novamente...\n");
	}


  return;
}

void RejectFifo() {
	if (mkfifo(rej_fifo, S_IRUSR | S_IWUSR) != 0) {
		if (errno == EEXIST)
			printf("Sauna info: FIFO de rejeicao ja existe\n");
		else
			printf("Sauna info: Impossivel criar FIFO rejeicao.");
	}
	//else printf("Sauna info: FIFO rejeicao criado.\n");

	while ((REJ_FIFO_FD = open(rej_fifo, O_WRONLY | O_NONBLOCK)) == -1) {
		//printf("Sauna info: Tentando abrir FIFO rejeicao...\n");
	}

  return;
}


int main(int argc, char* argv[]){

  gettimeofday(&t0, NULL);

  if(argc != 2){
    printf("Sintax deve ser: sauna <n.lugares>\n");
    exit(-1);
  }

  capacidade = atoi(argv[1]);

  int pid;
  pid = getpid();
  char WPathname[20];
  sprintf(WPathname, "/tmp/bal.%d", pid);
  WFile = fopen(WPathname, "w");
  if (WFile == NULL)
	 printf("Erro!\n");
  openFifo();
  RejectFifo();
  read(ENT_FIFO_FD, &faltamLer, sizeof(int));
  Receptor();
  for (int n = 0; n < 255; n++)
  {
	pthread_join(threadsTid[n], NULL);
  }
  printStats();
  fclose(WFile);
  unlink(rej_fifo);
  exit(0);

}
