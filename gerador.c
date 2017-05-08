#include "request.h"

Request* generateRequest(int duration){

    Request* request = malloc(sizeof(Request));

    request->id = ID++;
    request->gender = (rand()%2) ? 'M' : 'F';
    request->duration = rand() % duration + 1;
    request->denials = 0;

    return request;
}


int main(int argc, char* argv[]){

  if(argc != 3){
    printf("Sintax must be: gerador <numero de pedidos> <max.utilizacao>\n");
    exit(-1);
  }






}
