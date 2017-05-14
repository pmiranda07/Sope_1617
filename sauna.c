#include "request.h"

FILE* WFile;
int capacidade;
int rec_F, rec_M, rej_F, rej_M, done_F, done_M;
char genderUsing="N";
struct timeval t0;
int faltamLer;
int ENT_FIFO_FD;
int REJ_FIFO_FD;
int ocupantes=0;
pthread_t threadsTid[255];
int threadPos=0;

void printStats(){

    printf("...Sauna Requests Receive...\n");
    printf("M: %d\n", rec_M);
    printf("F: %d\n", rec_F);

    printf("...Sauna Requests Served...\n");
    printf("M: %d\n", done_M);
    printf("F: %d\n", done_F);

    printf("...Sauna Requests Rejected...\n");
    printf("M: %d\n", rej_M);
    printf("F: %d\n", rej_F);
}

void writeToFile(Request *req, int tid, char* tip){

  struct timeval t1;
  gettimeofday(&t1, NULL);
  double dt = (double)(t1.tv_usec - t0.tv_usec)/100;

  fprintf(WFile, "%-9.2f - %-4d - %-12d - %-4d: %-1c - %-4d - %-10s\n", dt, getpid(), tid ,req->id,req->gender, req->duration, tip);

  if(request->gender=='M'){
    if(strcmp(tip,"REJECT")==0) rej_M++;
    if(strcmp(tip,"RECEIVE")==0) rec_M++;
    if(strcmp(tip,"SERVED")==0) done_M++;
  }
  else{
    if(strcmp(tip,"REJECT")==0) rej_M++;
    if(strcmp(tip,"RECEIVE")==0) rec_M++;
    if(strcmp(tip,"SERVED")==0) done_M++;
  }

}

void DealReject(Request* req){

    req->denials = req->denials + 1;

    if(req->denials < 3)
      faltamLer++;

    printf("Sauna reject: ID:%i-Gender:%c-Duration:%i-Denials:%i;\n", req->id, req->gender, req->duration, req->denials);
    write(REJ_FIFO_FD, req, sizeof(Request));
}

void *RequestStays(void *arg) {

  Request *req = (Request*)arg;

    printf("%d has entered sauna\n",req->id);
    printf("%d: %d\n", req->id, req->duration);
    usleep(req->duration*100);

    printFile(req, pthread_self(),"SERVED");

    ocupantes--;

    printf("%d has exited sauna\n",req->id);

    printf("Sauna occupation: %d\n", ocupantes);

    if(ocupantes==0){
      genderUsing = 'N';
      printf("Sauna is empty,any gender is allow);
    }

    pthread_exit(NULL);
}


void DealRequest(Request* req){


            if(genderUsing=='N') {
                      genderUsing = req->gender;
                      printf("Sauna allowed gender: %c\n",genderUsing);
                      printf("Sauna served: ID:%i-Gender:%c-Duration:%i-Denials:%i;\n", req->id, req->gender, req->duration, req->denials);
                      ocupantes++;
                      writeToFile(req, getpid(),"RECEIVE");
                      pthread_create(&threadsTid[threadPos], NULL,RequestStays,req);
                      threadPos++;
            } else {
                      if(req->gender == genderUsing && ocupantes < capacidade) {
                          printf("Sauna served:ID:%i-Gender:%c-Duration:%i-Denials:%i;\n", req->id, req->gender, req->duration, req->denials);
                          ocupantes++;
                          WriteToFile(req, getpid(),"RECEIVE");
                          pthread_create(&threadsTid[threadPos], NULL, RequestStays,req);
                          threadPos++;
                      } else {
                          WriteToFile(req, getpid(),"RECEIVE");
                          WriteToFile(req, getpid(), "REJECT");
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
                  printf("Sauna requests left to read: %d\n", faltamLer);
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
			printf("Erro accessing to fifo exist! Retrying...\n");
	}

	printf("fifo entrance openned in READONLY mode\n");

  return;
}

void RejectFifo() {
	if (mkfifo(rej_fifo, S_IRUSR | S_IWUSR) != 0) {
		if (errno == EEXIST)
			printf("Reject fifo already exists\n");
		else
			printf("Can't create FIFO");
	}
	else printf("Reject fifo created\n");

	while ((REJ_FIFO_FD = open(rej_fifo, O_WRONLY | O_NONBLOCK)) == -1) {
		printf("Trying to open Reject fifo\n");
	}
	printf("Reject fifo opened in WRITEONLY mode\n");

  return;
}


int main(int argc, char* argv[]){

  gettimeofday(&t0, NULL);

  if(argc != 2){
    printf("Sintax must be: sauna <n.lugares>\n");
    exit(-1);
  }

  capacidade = atoi(argv[1]);

  int pid;
  pid = getpid();
  char WPathname[20];
  sprintf(WPathname, "/tmp/bal.%d", pid);
  WFile = fopen(WPathname, "w");
  if (WFile == NULL)
	printf("Error\n");
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
