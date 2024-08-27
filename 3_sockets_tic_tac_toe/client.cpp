#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

using namespace std;

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char message[2] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Falha ao criar o socket" << endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        cerr << "Endereço inválido/Endereço não suportado" << endl;
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Falha na conexão" << endl;
        return -1;
    }

    while (true) {
        // Recebe mensagem do servidor
        memset(buffer, 0, BUFFER_SIZE);
        read(sock, buffer, BUFFER_SIZE);
        cout << "Servidor: " << buffer << endl;

        // Se o servidor indicar que o jogo acabou, saia
        // if (strstr(buffer, "venceu") || strstr(buffer, "perdeu") ||
        // strstr(buffer, "Deu velha")) {
        // break;
        // }

        // Verifica se é a vez do jogador
        // if (strstr(buffer, "vez do Player")) {
        cout << "Digite o movimento (1-9): ";
        cin >> message[0];
        message[1] = '\0';
        send(sock, message, strlen(message), 0);
        // }
    }

    close(sock);
    return 0;
}
