#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 54321

int main() {
    // Criação do socket
    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Erro ao criar o socket\n";
        return 1;
    }

    // Configuração do endereço do servidor
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(serverAddress.sin_addr)) <= 0) {
        std::cerr << "Erro ao configurar o endereço do servidor\n";
        close(clientSocket);
        return 1;
    }

    // Leitura do arquivo de respostas
    std::ifstream inputFile("respostas.txt");
    if (!inputFile.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de respostas\n";
        close(clientSocket);
        return 1;
    }

    std::string line;
    while (std::getline(inputFile, line)) {
        // Envio das respostas ao servidor
        if (sendto(clientSocket, line.c_str(), line.length(), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
            std::cerr << "Erro ao enviar as respostas\n";
            close(clientSocket);
            inputFile.close();
            return 1;
        }

        // Aguarda a resposta do servidor
        char buffer[1024];
        socklen_t serverAddressLength = sizeof(serverAddress);
        ssize_t bytesRead = recvfrom(clientSocket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&serverAddress, &serverAddressLength);
        if (bytesRead == -1) {
            std::cerr << "Erro ao receber a resposta do servidor\n";
            close(clientSocket);
            inputFile.close();
            return 1;
        }

        buffer[bytesRead] = '\0';
        std::cout << "Resposta do servidor: " << buffer << std::endl;
    }

    inputFile.close();
    close(clientSocket);
    return 0;
}
