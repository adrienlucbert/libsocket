#include "../Socket.hpp"

int main(void)
{
    Socket server;

    server.listen(htons(1212), INADDR_ANY, SOMAXCONN);
    Socket client = server.accept();
    client.write("hello world!\n", 13);
    return 0;
}