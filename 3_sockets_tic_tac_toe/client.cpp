#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <thread>

using namespace std;

#define PORT 8080
#define BUFFER_SIZE 1024

// Função para receber mensagens do servidor
void receiveMessages(int sock) {
    char buffer[BUFFER_SIZE] = {0};
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(sock, buffer, BUFFER_SIZE);
        if (valread <= 0) {
            cerr << "Conexão perdida." << endl;
            break;
        }
        cout << "Servidor: " << buffer << endl;

        // Checar se o jogo acabou
        if (strstr(buffer, "venceu") || strstr(buffer, "perdeu") ||
            strstr(buffer, "Deu velha")) {
            cout << "Fim de jogo!" << endl;
            break;
        }
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

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

    // thread para receber mensagens do server
    thread recvThread(receiveMessages, sock);

    // Thread principal para enviar
    while (true) {
        char message[2] = {0};
        cout << "Digite o movimento (1-9): ";
        cin >> message[0];
        message[1] = '\0';

        send(sock, message, strlen(message), 0);

        if (strchr(message, 'sair')) {
            cout << "Saindo do jogo..." << endl;
            break;
        }
    }

    recvThread.join();

    close(sock);
    return 0;
}
