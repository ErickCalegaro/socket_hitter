#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <curl/curl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <ctype.h>
#include <pthread.h>

void *connection_handler(void *);

#define MAX_MSG 1024

struct MemoryStruct {
  char *memory;
  size_t size;
};
 
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(!ptr) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
 
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}


/*
 Servidor aguarda por mensagem do cliente, imprime na tela
 e depois envia resposta e finaliza processo
 */

int main(int argc, char* argv[]) {

    //Variaveis auxiliares para encontrar o arquivo a ser transferido.
    struct dirent *myfile;
    struct stat mystat;
    CURL *curl;
    CURL *curl_handle;
    CURLcode res;

    //verificando se foi executando o comando corretamente
    if (argc != 2) {
        fprintf(stderr, "use:./server [Porta]\n");
        return -1;
    } else if (!isdigit(*argv[1])) {
        fprintf(stderr, "Argumento invalido '%s'\n", argv[1]);
        fprintf(stderr, "use:./server [Porta]\n");
        return -1;
    }

    char* aux1 = argv[1];
        int portaServidor = atoi(aux1);

   //variaveis
    int socket_desc, conexao, c, nova_conex;
    struct sockaddr_in servidor, cliente;
    char *mensagem;
    char resposta[MAX_MSG];
    char postMessage[MAX_MSG] = {0x00};
    int tamanho, count;

    struct MemoryStruct chunk;
 
    chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
    chunk.size = 0;    /* no data at this point */

    // para pegar o IP e porta do cliente  
    char *cliente_ip;
    int cliente_port;

    //*********************************************************************//
    //      INICIO DO TRATAMENTO DA THREAD, localização e transferência    // 
    //      do arquivo.                                                    // 
    //*********************************************************************//
    void *connection_handler(void *socket_desc) {
        /*********************************************************/

        /*********comunicao entre cliente/servidor****************/

        // pegando IP e porta do cliente
        cliente_ip = inet_ntoa(cliente.sin_addr);
        cliente_port = ntohs(cliente.sin_port);
        printf("cliente conectou: %s : [ %d ]\n", cliente_ip, cliente_port);

        // lendo dados enviados pelo cliente
        //mensagem 1 recebido nome do arquivo   
        if ((tamanho = read(conexao, resposta, MAX_MSG)) < 0) {
            perror("Erro ao receber dados do cliente: ");
            return NULL;
        }
        resposta[tamanho] = '\0';
        printf("O cliente falou: %s\n", resposta);

        //Aqui eu recebo os dados da transação
        // char send_data[] = MODALITY_DEBIT "01" "000000000100";

        char trans_Data[MAX_MSG];
        //fazendo copia do nome do arquivo para variavel auxiliar. tal variavel é utilizada para localizar
        // o arquivo no diretorio.
        strncpy(trans_Data, resposta, MAX_MSG);
        printf("trans_Data: %s\n", trans_Data);

        /*********************************************************/
        if (strlen(trans_Data) == 16) {
            /* In windows, this will init the winsock stuff */
            curl_global_init(CURL_GLOBAL_ALL);

            /* get a curl handle */
            curl = curl_easy_init();
            if(curl) {
                /* First set the URL that is about to receive our POST. This URL can
                   just as well be a https:// URL if that is what should receive the
                   data. */
                curl_easy_setopt(curl, CURLOPT_URL, "ttp://tinywebdb.appinventor.mit.edu/storeavalue");

                /* Now specify the POST data */
                sprintf(postMessage, "tag=qrcode_mp30&value=%s&fmt=html", trans_Data);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postMessage);

                /* Perform the request, res will get the return code */
                res = curl_easy_perform(curl);
                /* Check for errors */
                if(res != CURLE_OK){
                  fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                }

                /* always cleanup */
                curl_easy_cleanup(curl);
            }

            curl_global_cleanup();

            if (res == CURLE_OK){
              mensagem = "200";
              write(conexao, mensagem, strlen(mensagem));
            }else{
              mensagem = "400";
              write(conexao, mensagem, strlen(mensagem));
              close(conexao);
              return NULL;
            }

            curl_global_init(CURL_GLOBAL_ALL);

            /* init the curl session */
            curl_handle = curl_easy_init();

            if(curl_handle) {
                /* specify URL to get */
                curl_easy_setopt(curl_handle, CURLOPT_URL, "http://tinywebdb.appinventor.mit.edu/getvalue");

                /* send all data to this function  */
                curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

                /* we pass our 'chunk' struct to the callback function */
                curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

                /* some servers do not like requests that are made without a user-agent
                 field, so we provide one */
                curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

                /* Now specify the POST data */
                curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, "tag=qrcode_mp30_approved&fmt=html");

                for (; count < 30; count++)
                {   
                    free(chunk.memory);
                    chunk.size = 0;
                    /* get it! */
                    res = curl_easy_perform(curl_handle);

                    /* check for errors */
                    if(res != CURLE_OK) {
                        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                    }else {
                        printf("%lu bytes retrieved\n", (unsigned long)chunk.size);
                        printf("%s\n", chunk.memory);
                        if (strncmp(chunk.memory, "TRUE", 4) == 0){
                            break;
                        }
                    }
                    sleep(1000);
                }


                /* cleanup curl stuff */
                curl_easy_cleanup(curl_handle);
            }

            /* we are done with libcurl, so clean it up */
            curl_global_cleanup();

        }

        if (count == 30){
          mensagem = "400";
          write(conexao, mensagem, strlen(mensagem));
        }else{
          mensagem = "200";
          write(conexao, mensagem, strlen(mensagem));
        }

        close(conexao);
        printf("Servidor finalizado...\n");
        return NULL;

    }

    //*********************************************************************//
    //      FIM DO TRATAMENTO DA THREAD, localização e transferencia    // 
    //      do arquivo.                                                    // 
    //*********************************************************************//


    //************************************************************
    /*********************************************************/
    //Criando um socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Nao foi possivel criar o socket\n");
        return -1;
    }

    //Preparando a struct do socket
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY; // Obtem IP do S.O.
    servidor.sin_port = htons(portaServidor);

    //Associando o socket a porta e endereco
    if (bind(socket_desc, (struct sockaddr *) &servidor, sizeof (servidor)) < 0) {
        puts("Erro ao fazer bind Tente outra porta\n");
        return -1;
    }
    puts("Bind efetuado com sucesso\n");

    // Ouvindo por conexoes
    listen(socket_desc, 3);
    /*********************************************************/

    //Aceitando e tratando conexoes

    puts("Aguardando por conexoes...");
    c = sizeof (struct sockaddr_in);

    while ((conexao = accept(socket_desc, (struct sockaddr *) &cliente, (socklen_t*) & c))) {
        if (conexao < 0) {
            perror("Erro ao receber conexao\n");
            return -1;
        }

        pthread_t sniffer_thread;
        nova_conex = (int) malloc(1);
        nova_conex = conexao;

        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void*) nova_conex) < 0) {
            perror("could not create thread");
            return 1;
        }
        puts("Handler assigned");
    }
    if (nova_conex < 0) {
        perror("accept failed");
        return 1;
    }
}