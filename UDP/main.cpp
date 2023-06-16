#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <sstream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define SERVER_PORT 54321

// Estrutura para armazenar estatísticas de questões
struct QuestionStats {
    int acertos = 0;
    int erros = 0;
};

// Objeto compartilhado por todas as threads para armazenar as estatísticas
std::unordered_map<int, QuestionStats> questionStats;
std::mutex mutex; // Mutex para garantir acesso seguro às estatísticas

// Função que trata as requisições dos clientes
void handleRequest(const std::string& request) {
    std::istringstream iss(request);
    std::string token;

    // Extrai os campos do datagrama
    std::getline(iss, token, ';');
    int numeroQuestao = std::stoi(token);

    std::getline(iss, token, ';');
    int numeroAlternativas = std::stoi(token);

    std::getline(iss, token, ';');
    std::string respostas = token;

    // Calcula os acertos e erros
    int acertos = 0;
    int erros = 0;

    for (int i = 0; i < respostas.size(); i++) {
        if (respostas[i] == 'V') {
            acertos++;
        } else if (respostas[i] == 'F') {
            erros++;
        }
    }

    // Atualiza as estatísticas da questão
    {
        std::lock_guard<std::mutex> lock(mutex);
        questionStats[numeroQuestao].acertos += acertos;
        questionStats[numeroQuestao].erros += erros;
    }

    // Envia a resposta ao cliente
    std::string response = std::to_string(numeroQuestao) + ";" + std::to_string(acertos) + ";" + std::to_string(erros);

    // Criação do socket UDP para enviar a resposta ao cliente
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Erro ao criar socket" << std::endl;
        return;
    }

    // Definição das informações do cliente
    struct sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(SERVER_PORT); // Porta do cliente
    clientAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Endereço IP do cliente (neste exemplo, assume-se o localhost)

    // Envio da resposta ao cliente
    ssize_t bytesSent = sendto(sockfd, response.c_str(), response.size(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
    if (bytesSent < 0) {
        std::cerr << "Erro ao enviar resposta" << std::endl;
    } else {
        // Exibe a resposta enviada ao cliente
        std::cout << "Resposta enviada: " << response << std::endl;
    }

    close(sockfd); // Fechamento do socket
}

int main() {
    // Criação do socket UDP
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Erro ao criar socket" << std::endl;
        return 1;
    }

    // Definição das informações do servidor
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Associação do socket à porta do servidor
    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Erro ao associar socket à porta" << std::endl;
        close(sockfd);
        return 1;
    }

    // Loop principal para receber e tratar as conexões dos clientes
    std::vector<std::thread> threads; // Vetor para armazenar as threads de atendimento aos clientes
    const int maxRequests = 1; // Limite máximo de requisições a serem tratadas

    while (threads.size() < maxRequests) {
        // Recebe o datagrama do cliente
        char buffer[BUFFER_SIZE];
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);

        ssize_t bytesRead = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (bytesRead < 0) {
            std::cerr << "Erro ao receber datagrama" << std::endl;
            continue;
        }

        // Converte o buffer em uma string
        buffer[bytesRead] = '\0';
        std::string request(buffer);

        // Trata a requisição em uma nova thread
        std::thread t(handleRequest, request);
        threads.push_back(std::move(t));
    }

    // Aguarda todas as threads finalizarem
    for (std::thread& t : threads) {
        t.join();
    }

    // Fechamento do socket
    close(sockfd);

    // Exibe as estatísticas das questões
    for (const auto& pair : questionStats) {
        int numeroQuestao = pair.first;
        const QuestionStats& stats = pair.second;
        std::cout << "Questão " << numeroQuestao << ": acertos=" << stats.acertos << " erros=" << stats.erros << std::endl;
    }

    return 0;
}
