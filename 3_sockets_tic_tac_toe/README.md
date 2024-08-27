g++ server.cpp -o server
g++ client.cpp -o client

./server
./client



Para rodar o dockerfile:


docker build -t tic-tac-toe-server .
docker run -d -p 1303:1303 tic-tac-toe-server