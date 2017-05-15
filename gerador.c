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

void printOnScreen(){

  printf("\n\n ------ RESUMO ------ \n\n");

    printf(" -- GERADOR PEDIDOS --\n");
    printf(" > M: %d\n", gen_M);
    printf(" > F: %d\n", gen_F);
    printf(" > TOTAL: %d\n\n", gen_M + gen_F);

    printf(" -- GERADOR REJEITADOS --\n");
    printf(" > M: %d\n", rej_M);
    printf(" > F: %d\n", rej_F);
    printf(" > TOTAL: %d\n\n", rej_M + rej_F);

    printf(" -- GERADOR DESCARTADOS --\n");
    printf(" > M: %d\n", del_M);
    printf(" > F: %d\n", del_F);
    printf(" > TOTAL: %d\n\n", del_M + del_F);
}

void printToFile(Request *request, char* tipo){

  struct timeval fim;
  gettimeofday(&fim, NULL);
  double instante = (fim.tv_sec - inicio.tv_sec)*1000.0f + (fim.tv_usec - inicio.tv_usec) / 1000.0f;//milissegundos depois do inicio do programa

  fprintf(ficheiroGer, "%-6.2f - %-4d - %-4d: %-1c - %-4d - %-10s\n", instante, getpid() ,request->id,request->gender, request->duration, tipo);

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
    if (errno == ENOENT) //printf("Fifo de rejeitados nao disponivel. Tentando novamente...\n");
    sleep(1);
  }

  while(read(FDFIFO_rejeitados, req, sizeof(Request)) != 0){
    if(req->id!=0){
      if(req->id==-1) break;
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

  if (mkfifo(gen_fifo, S_IRUSR | S_IWUSR) != 0) {
          if (errno == EEXIST)
                printf("Gerador info: FIFO '/tmp/entrada' already exists\n");
          else
                printf("Gerador info: Can't create FIFO '/tmp/entrada'\n");
      }

  while ((FDFIFO_sauna = open(gen_fifo, O_WRONLY | O_NONBLOCK)) == -1) {
        sleep(1);
  	}
  	return;
}


void openFifoPedidosRejeitados(){

  while ((FDFIFO_rejeitados = open(rej_fifo, O_RDONLY)) == -1) {
		if (errno == EEXIST)
      sleep(2);
	}

	return;
}



int main(int argc, char* argv[]){

  gettimeofday(&inicio , NULL);

  time_t t;
  srand((unsigned) time(&t));

  //verificar argumentos
  if(argc != 3){
    printf("Sintax deve ser: gerador <numero de pedidos> <max.utilizacao>\n");
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
