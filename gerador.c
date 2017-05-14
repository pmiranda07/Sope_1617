#include "request.h"
#include <pthread.h>

FILE* ficheiroGer;

struct timeval inicio;

int nrequests; // numero de pedidos
int duracaoMax; // tempo maximo

int id=1;

int FDFIFO_sauna; //FD do FIFO de entrada
int FDFIFO_rejeitados; //FD do FIFO de rejeitados


int gen_M=0, gen_F=0, rej_M=0, rej_F=0, del_M=0, del_F=0;
char* msg;

void printOnScreen(){
    printf("--PEDIDOS--\n");
    printf("M: %d\n", gen_M);
    printf("F: %d\n", gen_F);
    printf("T: %d\n", gen_M + gen_F);

    printf("--REJEITADOS--\n");
    printf("M: %d\n", rej_M);
    printf("F: %d\n", rej_F);
    printf("T: %d\n", rej_M + rej_F);

    printf("--DESCARTADOS--\n");
    printf("M: %d\n", del_M);
    printf("F: %d\n", del_F);
    printf("T: %d\n", del_M + del_F);
}

void printToFile(Request *request, char* tipo){

  struct timeval fim; //struct que guarda a hora do instante pretendido
  gettimeofday(&fim, NULL);
  double inst = (double)(fim.tv_usec - inicio.tv_usec)/1000; //milissegundos depois do inicio do programa

  fprintf(ficheiroGer, "%-6.2f - %-4d - %-4d: %-1c - %-4d - %-10s\n", inst, getpid() ,request->id,request->gender, request->duration, tipo);

  if(request->gender=='M'){
    if(strcmp(tipo,"PEDIDO")==0) gen_M++;
    if(strcmp(tipo,"REJEITADO")==0) rej_M++;
    if(strcmp(tipo,"DESCARTADO")==0) del_M++;
  }
  else{
    if(strcmp(tipo,"PEDIDO")==0) gen_F++;
    if(strcmp(tipo,"REJEITADO")==0) rej_F++;
    if(strcmp(tipo,"DESCARTADO")==0) del_F++;
  }

}


void* request_gen(void* arg){

  int numr = *(int*) arg;

  int i;
  for (i = 0; i < numr; i++){
    Request* req = malloc(sizeof(Request));

    req->id = id++;
    req->gender = (rand() % 2) ? 'M' : 'F';
    req->duration = rand() % duracaoMax + 1;
    req->denials = 0;

    write(FDFIFO_sauna, req, sizeof(Request));
    printToFile(req, "PEDIDO");

  }
  pthread_exit(NULL);
}

void* rejLis(void* arg){

	Request* req = malloc(sizeof(Request));

  while ((FDFIFO_rejeitados = open(rej_fifo, O_RDONLY)) == -1){
    if (errno == ENOENT) printf("Fifo de rejeitados nao disponivel. Tentando novamente...Retrying...\n");
    sleep(2);
  }

  while(read(FDFIFO_rejeitados, req, sizeof(Request)) != 0){
    if(req->id!=0){
      if(req->id==-1) break;
        printf("Gerador info (rejeitado): P:%i-G:%c-T:%i-D:%i;\n", req->id, req->gender, req->duration, req->denials);
        if(req->denials<3) {
            printToFile(req, "REJEITADO");
            write(FDFIFO_sauna, req, sizeof(Request));
            printToFile(req, "PEDIDO");
            }
        else printToFile(req, "DESCARTADO");
      }
  }
  pthread_exit(NULL);
}

void openFifoEntradaSauna(){

  while ((FDFIFO_sauna = open("/tmp/entrada", O_WRONLY | O_NONBLOCK)) == -1) {
  		printf("Gerador info: a espera que sauna abra fifo...\n");
        sleep(2);
  	}
  	printf("Gerador info: fifo de entrada aberta em modo WRITEONLY\n");
    sleep(2);
  	return;
}


void openFifoPedidosRejeitados(){

  while ((FDFIFO_rejeitados = open(rej_fifo, O_RDONLY)) == -1) {
		if (errno == EEXIST)
			printf("Gerador info: fifo de pedidos rejeitados nao existe. A tentar novamente...\n");
      sleep(2);
	}

	printf("Gerador info: fifo de pedidos rejeitados aberto em modo READONLY\n");
	return;
}



int main(int argc, char* argv[]){

  gettimeofday(&inicio , NULL);

  time_t t;
  srand((unsigned) time(&t));

  //verificar argumentos
  if(argc != 3){
    printf("Sintax must be: gerador <numero de pedidos> <max.utilizacao>\n");
    exit(-1);
  }

  //guardar nas variaveis globais
  nrequests = atoi(argv[1]);
  duracaoMax = atoi(argv[2]);

  //ficheiro de registo
  char pid[32], gerStr[64] = "/tmp/ger.";
  sprintf(pid, "%d", getpid());
  strcat(gerStr, pid);
  ficheiroGer = fopen(gerStr, "w");

  if (ficheiroGer == NULL)
		printf("Gerador (info): Erro a abrir ficheiro do Gerador.\n");

  //fifos
  openFifoEntradaSauna();
  openFifoPedidosRejeitados();

  //Escrever numero de pedidos
	write(FDFIFO_sauna, &nrequests, sizeof(int));

  //Thread para gerar os pedidos aleatorios
  pthread_t gen_tid;
  pthread_create(&gen_tid, NULL, request_gen, (void*) &nrequests);

  //Thread para os pedidos rejeitados
  pthread_t req_tid;
  pthread_create(&req_tid, NULL, rejLis, NULL);

  pthread_join(gen_tid, NULL);
  pthread_join(req_tid, NULL);

  printOnScreen();
  unlink(gen_fifo);

  return 0;

}
