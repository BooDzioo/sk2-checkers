#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <poll.h>
#include <utility>
#include <limits>
#include <thread>

#define PORT 8080
#define BUFFER_SIZE 1024

struct Client {
    int socket;
    std::string name;

    Client(int socket) : socket(socket) {}
};

class Checker {
public:
    Checker(const std::string& color, const std::pair<int, int>& position)
            : color(color), position(position) {}

    std::string getColor() const {
        return color;
    }

    std::pair<int, int> getPosition() const {
        return position;
    }

    std::pair<int, int> position;
private:
    std::string color;
};

class Board {
public:
    std::string currentTurn = "white";

    bool handleMove(std::pair<std::string, std::string> movePair, std::string color) {
        try {
            if(color != currentTurn) {
                throw std::invalid_argument("");
            }

            std::string firstPositionRow = movePair.first.substr(0, 1);
            std::string firstPositionColumn = movePair.first.substr(1, 1);
            std::pair<int, int> firstPosition = std::make_pair(stoi(firstPositionRow), stoi(firstPositionColumn));

            size_t index = findCheckerIndexAtPosition(checkers, firstPosition);
            if(checkers[index].getColor() != color) {
                throw std::invalid_argument("Cannot move opponent's checker");
            }

            std::string secondPositionRow = movePair.second.substr(0, 1);
            std::string secondPositionColumn = movePair.second.substr(1, 1);
            std::pair<int, int> secondPosition = std::make_pair(stoi(secondPositionRow), stoi(secondPositionColumn));

            try {
                size_t targetIndex = findCheckerIndexAtPosition(checkers, secondPosition);
                throw std::invalid_argument("Target position is occupied");
            } catch(std::invalid_argument& error) {
            }

            int rowDiff = secondPosition.first - firstPosition.first;
            int colDiff = secondPosition.second - firstPosition.second;

            if (std::abs(rowDiff) == 2 && std::abs(colDiff) == 2) {
                std::pair<int, int> capturedPosition = std::make_pair(
                        firstPosition.first + rowDiff / 2,
                        firstPosition.second + colDiff / 2
                );

                size_t capturedIndex = findCheckerIndexAtPosition(checkers, capturedPosition);

                if (checkers[capturedIndex].getColor() == (color == "white" ? "black" : "white")) {
                    checkers[index].position = secondPosition;
                    checkers.erase(checkers.begin() + capturedIndex);

                    currentTurn = (color == "white") ? "black" : "white";
                } else {
                    throw std::invalid_argument("Invalid capture, must capture opponent's checker");
                }
            } else if (std::abs(rowDiff) == 1 && std::abs(colDiff) == 1) {
                checkers[index].position = secondPosition;
                currentTurn = (color == "white") ? "black" : "white";
            } else {
                throw std::invalid_argument("Invalid move, can only move one diagonal step or capture opponent's checker");
            }

        } catch(std::invalid_argument& error) {
            return false;
        }

        return true;
    }

    void addChecker(const Checker& checker) {
        checkers.push_back(checker);
    }

    void generateInitialBoard() {
        for(int i = 0; i < 8; i++) {
            for(int j = 0; j < 8; j++) {
                if(i < 3) {
                    if((i % 2 == 0 && j % 2 == 1) || (i % 2 == 1 && j % 2 == 0)) {
                        addChecker(Checker("white", std::make_pair(i, j)));
                    }
                }

                if(i > 4) {
                    if((i % 2 == 0 && j % 2 == 1) || (i % 2 == 1 && j % 2 == 0)) {
                        addChecker(Checker("black", std::make_pair(i, j)));
                    }
                }

            }
        }
    }

    size_t findCheckerIndexAtPosition(const std::vector<Checker>& checkers, const std::pair<int, int>& position) {
        for (size_t i = 0; i < checkers.size(); ++i) {
            const auto& checker = checkers[i];
            if (checker.getPosition() == position) {
                return i;
            }
        }

        throw std::invalid_argument("");
    }

    std::string toString() {
        std::string result = "| ";

        for(int j = 0; j < 8; j++) {
            result += "|" + std::to_string(j);
        }
        result += "\n";

        for(int i = 0; i < 8; i++) {
            result += "|" + std::to_string(i);
            for(int j = 0; j < 8; j++) {
                try {
                    size_t index = findCheckerIndexAtPosition(checkers, std::make_pair(i, j));

                    if(checkers[index].getColor() == "white") {
                        result = result + "|w";
                    }

                    if(checkers[index].getColor() == "black") {
                        result = result + "|b";
                    }
                } catch(std::invalid_argument error) {
                    result = result + "|-";
                }
            }

            result = result + "\n";
        }

        return result;
    }

private:
    std::vector<Checker> checkers;
};

class Game {
public:
    Game(Client c1, Client c2, Board b) : client1(c1), client2(c2), board(b) {
    }

    Client client1;
    Client client2;
    Board board;

    bool handleMove(Client client, std::string move) {
        std::string firstPosition = move.substr(0, 2);
        std::string secondPosition = move.substr(3, 2);
        std::pair<std::string, std::string> movePair = std::make_pair(firstPosition, secondPosition);


        if(client.name == client1.name && board.currentTurn == "white") {
            return board.handleMove(movePair, "white");
        }

        if(client.name == client2.name && board.currentTurn == "black") {
            return board.handleMove(movePair, "black");
        }

        return false;
    }
};

bool clientIsInGame(const std::vector<Game>& games, const Client& client) {
    for(const auto& game : games) {
        if (game.client1.name == client.name || game.client2.name == client.name) {
            return true;
        }
    }

    return false;
}

size_t findGameIndexWithClient(const std::vector<Game>& games, const Client& client) {
    for (size_t i = 0; i < games.size(); ++i) {
        const auto &game = games[i];
        if (game.client1.name == client.name || game.client2.name == client.name) {
            return i;
        }
    }

    throw std::invalid_argument("");
}

void delay(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void sendMoveInfo(Client currentMoveClient, Client opponentMoveClient) {
    std::string message2 = "your_move";
    send(currentMoveClient.socket, message2.c_str(), message2.length(), 0);
    delay(100);
    std::string message3 = "opponent_move";
    send(opponentMoveClient.socket, message3.c_str(), message3.length(), 0);
    delay(100);
}

void sendBoardInfo(Game game, Client client1, Client client2) {
    std::string message = "print_board:" + game.board.toString();
    send(client1.socket, message.c_str(), message.length(), 0);
    delay(100);
    send(client2.socket, message.c_str(), message.length(), 0);
    delay(100);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding server socket");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("Error listening on server socket");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    std::vector<pollfd> fds;

    std::vector<Client> clients;
    std::vector<Game> games;

    pollfd server_pollfd;
    server_pollfd.fd = server_socket;
    server_pollfd.events = POLLIN;
    fds.push_back(server_pollfd);

    while (1) {
        if (poll(fds.data(), fds.size(), -1) == -1) {
            perror("Error in poll");
            exit(EXIT_FAILURE);
        }

        if (fds[0].revents & POLLIN) {
            if ((client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len)) == -1) {
                perror("Error accepting client connection");
                continue;
            }

            std::cout << "New connection accepted from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << std::endl;

            pollfd client_pollfd;
            client_pollfd.fd = client_socket;
            client_pollfd.events = POLLIN;
            fds.push_back(client_pollfd);

            clients.emplace_back(client_socket);
        }

        for (size_t i = 1; i < fds.size(); ++i) {
            if (fds[i].revents & POLLIN) {
                char buffer[BUFFER_SIZE];
                memset(buffer, 0, BUFFER_SIZE);

                int bytes_received = recv(fds[i].fd, buffer, BUFFER_SIZE, 0);

                if (bytes_received <= 0) {
                    std::cout << "Connection closed for client socket " << fds[i].fd << std::endl;

                    auto it = std::find_if(clients.begin(), clients.end(), [fd = fds[i].fd](const Client& client) {
                        return client.socket == fd;
                    });

                    if (it != clients.end()) {
                        try {
                            size_t gameIndex = findGameIndexWithClient(games, *it);

                            std::string message = "end:Przeciwnik zerwał połączenie, gra zakończona";
                            if(games[gameIndex].client1.name == it->name) {
                                send(games[gameIndex].client2.socket, message.c_str(), message.length(), 0);
                            } else {
                                send(games[gameIndex].client1.socket, message.c_str(), message.length(), 0);
                            }

                            games.erase(games.begin() + gameIndex);
                        } catch(std::invalid_argument error) {}


                        clients.erase(it);
                    }

                    close(fds[i].fd);
                    fds.erase(fds.begin() + i);
                    --i;
                    continue;
                }

                std::string received_message(buffer);
                Client client = clients[i - 1];

                std::cout << "received message: " << received_message << std::endl;

                if (received_message.substr(0, 3) == "N: ") {
                    clients[i - 1].name = received_message.substr(3);
                    std::cout << "Client [" << clients[i - 1].name << "] identified." << std::endl;

                    for(const auto client : clients) {
                        if(clients[i - 1].name != client.name && !clientIsInGame(games, client)) {
                            Game game = Game(clients[i - 1], client, Board());
                            game.board.generateInitialBoard();

                            games.push_back(game);

                            sendBoardInfo(game, clients[i - 1], client);

                            sendMoveInfo(clients[i - 1], client);
                            break;
                        }
                    }
                }

                if (received_message.substr(0, 5) == "move:") {
                    std::string move = received_message.substr(5);

                    try {
                        size_t index = findGameIndexWithClient(games, client);

                        bool result = games[index].handleMove(client, move);

                        if(!result) {
                            std::string message = "error:zły ruch";
                            send(client.socket, message.c_str(), message.length(), 0);

                            std::string message2 = "your_move";
                            send(client.socket, message2.c_str(), message2.length(), 0);
                        } else {
                            Client otherClient = games[index].client1;
                            if(games[index].client1.name == client.name) {
                                otherClient = games[index].client2;
                            } else {
                                otherClient = games[index].client1;
                            }

                            sendBoardInfo(games[index], otherClient, client);
                            sendMoveInfo(otherClient, client);
                        }
                    } catch(std::invalid_argument error) {

                    }

                }
            }
        }
    }

    close(server_socket);

    return 0;
}
