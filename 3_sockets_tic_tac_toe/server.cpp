#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#define PORT 1303
#define MAX_CLIENTS 2
#define SIZE 3

using namespace std;

enum class GAME_STATE {
    WAITING_PLAYERS,
    IN_PROGRESS,
    GAME_OVER,
};

int BUFFER_SIZE = 512;
struct sockaddr_in address;
int addrlen = sizeof(address);
vector<thread> client_threads;
int server_socket;
int client_socket_1 = -1, client_socket_2 = -1;

int current_player = 1;
char board[SIZE][SIZE] = {{' ', ' ', ' '}, {' ', ' ', ' '}, {' ', ' ', ' '}};
GAME_STATE game_state = GAME_STATE::WAITING_PLAYERS;

void sendMessage(int client_socket, string message) {
    send(client_socket, message.c_str(), message.length(), 0);
}

void endGame() {
    close(client_socket_1);
    close(client_socket_2);
    close(server_socket);
    exit(EXIT_SUCCESS);
}

void printBoard() {
    string board_message = "\n";
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            board_message += board[i][j];

            if (j < SIZE - 1) {
                board_message += " | ";
            }
        }

        if (i < SIZE - 1) {
            board_message += "\n---------\n";
        }
    }

    board_message += "\n\n";
    cout << board_message << endl;
    sendMessage(client_socket_1, board_message);
    sendMessage(client_socket_2, board_message);
}

void handleTurnChange() {
    switch (current_player) {
        case 1:
            current_player = 2;
            cout << "Vez do Player 2" << endl;
            sendMessage(client_socket_2,
                        "\033[32mSua vez!\033[0m\nDigite o movimento (1-9):\n");
            sendMessage(client_socket_1, "Aguarde sua vez...\n");
            printBoard();
            break;
        case 2:
            current_player = 1;
            cout << "Vez do Player 1" << endl;
            sendMessage(client_socket_2, "Aguarde sua vez...\n");
            sendMessage(client_socket_1,
                        "\033[32mSua vez!\033[0m\nDigite o movimento (1-9):\n");
            printBoard();
            break;
        default:
            cout << "Valor inválido para current_player.\n" << endl;
            endGame();
            break;
    }
}

int checkGameStatus() {
    // Verifica linhas
    for (int i = 0; i < 3; ++i) {
        if (board[i][0] != ' ' && board[i][0] == board[i][1] &&
            board[i][1] == board[i][2]) {
            return (board[i][0] == 'X') ? 1 : 2;
        }
    }

    // Verifica colunas
    for (int i = 0; i < 3; ++i) {
        if (board[0][i] != ' ' && board[0][i] == board[1][i] &&
            board[1][i] == board[2][i]) {
            return (board[0][i] == 'X') ? 1 : 2;
        }
    }

    // Verifica diagonais
    if (board[0][0] != ' ' && board[0][0] == board[1][1] &&
        board[1][1] == board[2][2]) {
        return (board[0][0] == 'X') ? 1 : 2;
    }
    if (board[0][2] != ' ' && board[0][2] == board[1][1] &&
        board[1][1] == board[2][0]) {
        return (board[0][2] == 'X') ? 1 : 2;
    }

    // Verifica se há espaços vazios (jogo ainda em andamento)
    for (const auto &row : board) {
        for (char cell : row) {
            if (cell == ' ') {
                return 0;
            }
        }
    }

    // deu velha
    return -1;
}

void handleGameStatus() {
    int gameStatus = checkGameStatus();

    switch (gameStatus) {
        case 1:
            cout << "\nJogador 1 venceu!\n" << endl;
            printBoard();
            sendMessage(client_socket_1, "\033[32mVocê venceu!\033[0m\n");
            sendMessage(client_socket_2, "\033[31mVocê perdeu!\033[0m\n");
            endGame();
            break;
        case 2:
            cout << "\nJogador 2 venceu!\n" << endl;
            printBoard();
            sendMessage(client_socket_1, "\033[31mVocê perdeu!\033[0m\n");
            sendMessage(client_socket_2, "\033[32mVocê venceu!\033[0m\n");
            endGame();
            break;
        case -1:
            printBoard();
            cout << "\nDeu velha!\n" << endl;
            sendMessage(client_socket_1, "Deu velha!\n");
            sendMessage(client_socket_2, "Deu velha!\n");
            endGame();
            break;
        default:
            handleTurnChange();
            break;
    }
}

void startGame() {
    cout << "Iniciando jogo!" << endl;
    current_player = 1;
    sendMessage(client_socket_1,
                "\n\033[32mJogo Iniciado!\n\033[0m\nDigite o "
                "movimento (1-9):\n");
    sendMessage(
        client_socket_2,
        "\n\033[32mJogo Iniciado!\033[0m\nAguarde a jogada do Player 1:\n");
    game_state = GAME_STATE::IN_PROGRESS;
    printBoard();
}

void handleClientActions(int client_socket, int client_number) {
    char buffer[BUFFER_SIZE] = {0};
    char playerKey = (client_number == 1) ? 'X' : 'O';

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(client_socket, buffer, BUFFER_SIZE);

        if (valread <= 0) {
            cout << "Cliente " << client_number << " desconectado" << endl;

            if (client_number == 1) {
                sendMessage(client_socket_2, "Cliente 1 desconectado.\n");
            } else {
                sendMessage(client_socket_1, "Cliente 2 desconectado.\n");
            }
            endGame();
            break;
        }

        if (game_state != GAME_STATE::IN_PROGRESS) {
            sendMessage(client_socket, "Aguarde pelo início da partida.\n");
            continue;
        }

        if (client_number != current_player) {
            sendMessage(client_socket, "Aguarde a vez do outro jogador.\n");
            continue;
        }

        int playerChoice = buffer[0] - '1';
        int row = playerChoice / SIZE;
        int col = playerChoice % SIZE;

        if (row >= 0 && row < SIZE && col >= 0 && col < SIZE &&
            board[row][col] == ' ') {
            board[row][col] = playerKey;
            handleGameStatus();
        } else {
            sendMessage(client_socket,
                        "Movimento inválido. Tente novamente.\n");
        }
    }
}

void handleClientConnection(int &clientSocket, int clientNumber) {
    cout << "Aguardando conexão do Cliente " << clientNumber << "\n" << endl;
    socklen_t address_length = sizeof(address);
    int new_socket =
        accept(server_socket, (struct sockaddr *)&address, &address_length);

    if (new_socket < 0) {
        perror("Falha ao aceitar conexão");
        exit(EXIT_FAILURE);
    }
    clientSocket = new_socket;
    cout << "Cliente " << clientNumber << " conectado!\n" << endl;

    if (clientNumber == 1) {
        sendMessage(clientSocket,
                    "\n\033[32mVocê é o jogador 1 (X).\033[0m\nAguarde pela "
                    "conexão do jogador 2 "
                    "(O)!\n\n");
    } else {
        sendMessage(clientSocket,
                    "Jogador 1 (X) já conectado.\n\033[32mVocê é o jogador 2 "
                    "(O)!\033[0m\n\n");
    }

    if (client_socket_1 != -1 && client_socket_2 != -1) {
        startGame();
    }
}

void signalHandler(int signum) {
    close(client_socket_1);
    close(client_socket_2);
    close(server_socket);  // Fecha o socket do servidor
    cout << "\nSocket fechado e interrupção do servidor concluída." << endl;
    exit(signum);  // Encerra o programa com o código do sinal
}

int main() {
    system("cls");
    system("clear");

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Nao foi possivel criar o socket");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, signalHandler);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Falha na ligação");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Falha ao colocar em modo de escuta");
        exit(EXIT_FAILURE);
    }

    cout << "Servidor está ouvindo na porta " << PORT << endl;

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_socket_1 == -1) {
            handleClientConnection(client_socket_1, 1);
        } else if (client_socket_2 == -1) {
            handleClientConnection(client_socket_2, 2);
        }
    }

    client_threads.push_back(thread(handleClientActions, client_socket_1, 1));
    client_threads.push_back(thread(handleClientActions, client_socket_2, 2));

    // Aguardar todas as threads
    for (auto &t : client_threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    close(client_socket_1);
    close(client_socket_2);
    close(server_socket);
    cout << "Servidor encerrado" << endl;

    return 0;
}
