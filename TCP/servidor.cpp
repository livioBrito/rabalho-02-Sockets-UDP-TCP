#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>

#define SERVER_PORT 54321

struct QuestionStats {
    int correct;
    int incorrect;

    QuestionStats() : correct(0), incorrect(0) {}
};

std::unordered_map<int, QuestionStats> statistics;
std::mutex statisticsMutex;

void processRequest(const char* request, sockaddr_in clientAddress) {
    // Parsing da requisição
    int questionNumber, numAlternatives;
    std::string answers;

    sscanf(request, "%d;%d;%s", &questionNumber, &numAlternatives, &answers[0]);

    // Verificação das respostas
    int numCorrect = 0, numIncorrect = 0;

    for (int i = 0; i < numAlternatives; ++i) {
        if (answers[i] == 'V' || answers[i] == 'v') {
            ++numCorrect;
        } else {
            ++numIncorrect;
        }
    }

    // Atualização das estatísticas
    {
        std::lock_guard<std::mutex> lock(statisticsMutex);
        statistics[questionNumber].correct += numCorrect;
        statistics[questionNumber].incorrect += numIncorrect;
    }

    // Preparação da resposta
    std::string response = std::to_string(questionNumber) + ";" + std::to_string(numCorrect) + ";" + std::to_string(numIncorrect) + "\n";

    // Envio da resposta ao cliente
    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Erro ao criar o socket do cliente\n";
        return;
    }

    ssize_t bytesSent = sendto(clientSocket, response.c_str(), response.size(), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
    if (bytesSent == -1) {
        std::cerr << "Erro ao enviar a resposta ao cliente\n";
    }

    close(clientSocket);
}

int main() {
    // Criação do socket do servidor
    int serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Erro ao criar o socket do servidor\n";
        return 1;
    }

    // Configuração do endereço do servidor
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(SERVER_PORT);

    // Associação do socket com o endereço do servidor
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Erro ao associar o socket com o endereço do servidor\n";
        close(serverSocket);
        return 1;
    }

    std::cout << "Servidor iniciado. Aguardando conexões...\n";

    // Loop principal de recebimento e processamento das requisições
    while (true) {
        char buffer[1024];
        sockaddr_in clientAddress{};
        socklen_t clientAddressLength = sizeof(clientAddress);

        ssize_t bytesRead = recvfrom(serverSocket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&clientAddress, &clientAddressLength);
        if (bytesRead == -1) {
            std::cerr << "Erro ao receber a requisição\n";
            continue;
        }

        buffer[bytesRead] = '\0';

        // Criação de uma thread para processar a requisição
        std::thread requestThread(processRequest, buffer, clientAddress);
        requestThread.detach();
    }

    close(serverSocket);
    return 0;
}
