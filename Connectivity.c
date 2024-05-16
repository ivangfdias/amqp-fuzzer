#include "Connectivity.h"

int connect_to_server(char* address, int port){

    int sockfd;
    struct sockaddr_in servaddr;
    

  // Cria um socket de internet do tipo stream usando o protocolo 0 (TCP)
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket :(\n");
    exit(2);
  }

  // Inicializa as informacoes do socket
  // Com a familia (Internet)
  // Com a porta (5672) - padrao para AMQP
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(5672); // traduz o 5672 pra entrar no negocio direito

  // Traduz um endereco ip e guarda nas informacoes do socket
  if (inet_pton(AF_INET, address, &servaddr.sin_addr) <= 0) {
    perror("inet_pton error");
    exit(2);
  }

  // Tenta conectar com esse socket
  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
    perror("bind :(\n");
    exit(3);
  }


  return sockfd;
}
