#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

/*! \brief Conecta para o servidor address na porta port
 * Retorna o socket_fd correspondente a essa conexao
 */
int connect_to_server(char* address, int port);

