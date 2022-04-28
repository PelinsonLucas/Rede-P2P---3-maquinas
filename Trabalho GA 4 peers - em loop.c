#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/ioctl.h>


#define QUANTPORTAS 4
#define ECHOMAX 255

int main(void)
{
    //Definicao de variaveis utilizadas
    char linha[ECHOMAX];
    pid_t pid;
    FILE *pipe;
    int sock;
    struct sockaddr_in server, client, otherPeer;
    int sock_size = sizeof(server);
    int value = 0;
    int portas[QUANTPORTAS];
    int porta;

    //Cria lista de portas
    portas[0] = 3333;
    portas[1] = 4444;
    portas[2] = 5555;
    portas[3] = 6666;

    //Criacao do socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        printf("ERRO na Criacao do Socket!\n");
    else
        printf("Esperando Mensagens...\n");

    //Define o socket como bloqueante
    ioctl(sock, FIONBIO, &value);

    //Cria o processo filho
    pid = fork();

    if (pid < 0)
        puts("Error creating process");
    else if (pid == 0){

        //Define variaveis utilizadas somente pelo processo de recebimento
        char resultadoComando[ECHOMAX];
        char linhaComando[ECHOMAX];

        while(1){
            //Recebe pelo socket
            recvfrom(sock, linha, ECHOMAX, 0,(struct sockaddr *)&otherPeer, &sock_size);
            //Caso o ultimo caractere seja um 'C', significa que recebeu um comando a ser executado
            if(linha[strlen(linha)-1] == 'C'){
                linha[strlen(linha)-1] = 0;

                //realiza a o comando e envia ele novamente a quem solicitou
                pipe = popen(linha, "r");
                if (pipe != NULL)
                {
                    while (fgets(linhaComando, sizeof(linhaComando), pipe) != NULL){
                        strcat(resultadoComando, linhaComando);
                    }
                    strncat(resultadoComando, "R", 1);
                    sendto(sock, resultadoComando, ECHOMAX, 0, (struct sockaddr *)&otherPeer, sock_size);
                    pclose(pipe);
                    resultadoComando[0] = '\0';
                }
            }
            //Caso o ultimo caractere seja um 'R', significa que recebeu uma resposta de comando solicitado
            else if (linha[strlen(linha)-1] == 'R'){
                linha[strlen(linha)-1] = 0;
                printf("Recebido do dispositivo de porta %d:\n%s\n", ntohs(otherPeer.sin_port) ,linha);
            }
        }

    }
    else{
        //Definicao dos enderecos do cliente (O dispositivo atual) e do servidor (Com quem quer se comunicar).
        bzero((char *)&client, sock_size);
        client.sin_family = AF_INET;
        client.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* endereco IP local */

        bzero((char *)&server, sock_size);
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* endereco IP local */
        //Tenta criar o socket para uma das portas na lista
        for(int x = 0; x < QUANTPORTAS; x++){

            client.sin_port = htons(portas[x]);
            if(bind(sock, (struct sockaddr *)&client, sock_size) != -1){
                
                porta = portas[x];
                break;
            }
        }
        if(porta == 0)
            puts("Sem portas disponiveis");

        //Envia o comando para os outros dispositivos    
        while(1){
            gets(linha);
            if(linha[0] != "\n"){
                printf("Executado localmente:\n");
                system(linha);
                printf("\n");
                linha[strcspn(linha, "\n")+1] = 0;
                linha[strcspn(linha, "\n")] = 'C';
                for(int x = 0; x < QUANTPORTAS; x++){

                    if(portas[x] != porta){
                        server.sin_port = htons(portas[x]); /* porta local  */
                        sendto(sock, linha, ECHOMAX, 0, (struct sockaddr *)&server, sock_size);                        
                    }
                }

                linha[0] = '\0';
            }
        }
    }
}


