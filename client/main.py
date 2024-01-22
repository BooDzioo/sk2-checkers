import socket


def main():
    server_ip = '127.0.0.1'
    server_port = 8080

    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    client_socket.connect((server_ip, server_port))

    print("Connected to the server.")

    username = input("Podaj nazwe użytkownika\n")

    client_socket.send(("N: " + username).encode())

    while True:
        message = client_socket.recv(4096).decode('utf-8')

        if 'end' in message:
            endMessage = message.split(":")[1]
            print(endMessage)
            exit()

        if 'error' in message:
            error = message.split(':')[1]

            print(error)
        if 'your_move' in message:
            move = input("Twój ruch (np. '12-23'):\n")

            client_socket.send(('move:' + move).encode('utf-8'))

        if 'opponent_move' in message:
            print("Oczekiwanie na ruch przeciwnika")

        if 'print_board' in message:
            board = message.split(':')[1]
            print(board)

    client_socket.close()


if __name__ == "__main__":
    main()
