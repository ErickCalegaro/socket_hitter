#ifndef HEADER_H
#define HEADER_H

//Constantes
#define ARGV_SERVER_PORT 	1 
#define ARGV_CLIENT_IP 		1 
#define ARGV_CLIENT_PORT 	2 

#define BOOL 				u_char
#define FALSE				0
#define TRUE				1

//Codigos de retorno
#define RC_OKAY 			0
#define RC_MANY_ARGS 		1
#define RC_FEW_ARGS 		2
#define RC_LOW_PORT_VALUE	3
#define RC_HIGH_PORT_VALUE	4
#define RC_SOCKET_CREATE	5
#define RC_SOCKET_OPTIONS	6
#define RC_SOCKET_ATACH		7
#define RC_SOCKET_LISTEN	8
#define RC_SOCKET_ACCEPT	9
#define RC_SOCKET_CONNECT	10
#define RC_INET_ADDRESS		11

//Prototipos Funcoes
int doProcessSocketServer	(char const *sPort);
int doProcessSocketClient	(char const *ip, char const *sPort);
int doProcessGame			(char * sRandNum);

//Prototipos Tratativas
int debugErrorsServer		(int rc);
int debugErrorsClient		(int rc);

#endif