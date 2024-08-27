g++ server.cpp -o server -pthread
g++ client.cpp -o client -pthread

./server
./client

