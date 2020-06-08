#include <iostream>

#include "../Socket.hpp"

int main(void)
{
    std::string buffer;
    Socket client;

    client.connect(htons(1212), INADDR_ANY);
    client.getline(buffer);
    std::cout << buffer << std::endl;
    return 0;
}