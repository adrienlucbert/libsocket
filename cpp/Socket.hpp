/*
* LibSocket C++ binding
* Header-only sockets manipulations wrapper
*/

#pragma once

#include <iostream>
#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

/**
 * @brief TCP socket wrapper
 */
class Socket : public std::ios
{
private:
    int _sd{ -1 };
    int _errno{ 0 };

public:
    /**
     * @brief Construct a new socket with the provided socket descriptor
     * @param sd socket descriptor
     */
    Socket(int sd)
        : _sd{ sd }
    {
        int state = 1;
        if (setsockopt(_sd, SOL_SOCKET, SO_REUSEADDR, &state, sizeof(state)) == -1)
            this->setstate(failbit);
        if (!*this)
            this->close();
        _errno = errno;
    }
    /**
     * @brief Construct a new socket
     */
    Socket()
        : Socket(socket(PF_INET, SOCK_STREAM, 0))
    {
    }
    Socket(const Socket &&other)
        : _sd{ std::move(other._sd) }
    {
    }
    ~Socket()
    {
        if (this->isOpen())
            this->close();
    }

    /**
     * @brief Listen to 'count' connections on given port and address
     * @param port port to listen to
     * @param addr address to listen to
     * @param count amount of connections to listen to
     */
    void listen(in_port_t port, in_addr_t addr, int count)
    {
        struct sockaddr_in st_addr = {
            .sin_family = AF_INET,
            .sin_port = port,
            .sin_addr = { .s_addr = addr }
        };
        socklen_t addrlen = sizeof(st_addr);

        if (this->good() && ::bind(_sd, (const struct sockaddr *)&st_addr, addrlen) == -1)
            this->setstate(failbit);
        if (this->good() && ::listen(_sd, count) == -1)
            this->setstate(failbit);
        _errno = errno;
    }
    /**
     * @brief Listen to 'count' connections on given port and address
     * @param port port to listen to
     * @param addr address (as a dot-separated string) to listen to
     * @param count amount of connections to listen to
     */
    void listen(in_port_t port, const char *addrstr, int count)
    {
        in_addr_t addr = 0;
        if (this->strToAddr(addrstr, &addr) == false)
            this->setstate(failbit);
        this->listen(port, addr, count);
    }

    /**
     * @brief Accept an incoming connection on current socket
     * @return connected client socket
     */
    Socket accept()
    {
        struct sockaddr_in st_addr = { 0 };
        socklen_t addrlen = sizeof(st_addr);

        int peersd = -1;
        if (this->good())
            peersd = ::accept(_sd, (struct sockaddr *)&st_addr, &addrlen);
        _errno = errno;
        return (Socket(peersd));
    }

    /**
     * @brief Connect to a remote address and port
     * @param port remote server port to connect to
     * @param addr remote server address to connect to
     */
    void connect(in_port_t port, in_addr_t addr)
    {
        struct sockaddr_in st_addr = {
            .sin_family = AF_INET,
            .sin_port = port,
            .sin_addr = { .s_addr = addr }
        };
        socklen_t addrlen = sizeof(st_addr);

        if (this->good() && ::connect(_sd, (const struct sockaddr *)&st_addr, addrlen) == -1)
            this->setstate(failbit);
        _errno = errno;
    }
    /**
     * @brief Connect to a remote address and port
     * @param port remote server port to connect to
     * @param addrstr remote server address (as a dot-separated string) to connect to
     */
    void connect(in_port_t port, const char *addrstr)
    {
        in_addr_t addr = 0;
        if (this->strToAddr(addrstr, &addr) == false)
            this->setstate(failbit);
        this->connect(port, addr);
    }

    /**
     * @brief Close socket connection
     */
    void close()
    {
        if (::close(_sd) == -1) {
            this->setstate(failbit);
        } else {
            this->setstate(goodbit);
        }
        _sd = -1;
        _errno = errno;
    }

    /**
     * @brief Check if socket is opened
     */
    inline bool isOpen() const
    {
        return (_sd != -1);
    }

    /**
     * @brief Get error code
     * @return getsockopt(2) error code or errno value
     */
    int errcode()
    {
        int optval = 0;
        socklen_t optlen = sizeof(optval);

        if (_errno != 0)
            return (_errno);
        if (getsockopt(_sd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) == -1)
            this->setstate(failbit);
        _errno = optval;
        return (_errno);
    }
    /**
     * @brief Return a string describing the last error that occured on socket
     */
    const char *strerror()
    {
        return (::strerror(this->errcode()));
    }

    /**
     * @brief Read from socket
     * @param buffer buffer to read data into
     * @param len maximum number of bytes to read
     */
    Socket &read(char *buffer, std::streamsize len)
    {
        ssize_t rdsize = ::read(_sd, buffer, len);
        _errno = errno;
        if (rdsize == 0 && len > 0)
            this->setstate(eofbit);
        if (rdsize == -1)
            this->setstate(badbit);
        return (*this);
    }
    /**
     * @brief Get line from socket
     * @param buffer buffer to read data into
     */
    Socket &getline(std::string &buffer, char delim = '\n')
    {
        char c = -1;
        buffer.clear();
        while (this->read(&c, 1) && c != delim && c != '\0')
            buffer += c;
        if (c == '\0')
            this->getline(buffer, delim);
        return (*this);
    }

    /**
     * @brief Write to socket
     * @param buffer buffer to write data from
     * @param len number of bytes to write
     */
    Socket &write(const char *buffer, std::streamsize len)
    {
        ssize_t wrsize = ::write(_sd, buffer, len);
        _errno = errno;
        if ((wrsize == 0 && len > 0) || wrsize == -1)
            this->setstate(badbit);
        return (*this);
    }

    /**
     * @brief Get info about the socket
     */
    const struct sockaddr_in &info()
    {
        static struct sockaddr_in st_addr;
        socklen_t addrlen = sizeof(st_addr);

        if (getsockname(_sd, (struct sockaddr *)&st_addr, &addrlen) == -1)
            this->setstate(failbit);
        return (st_addr);
    }
    /**
     * @brief Get info about the peer connect to the socket
     */
    const struct sockaddr_in &peerinfo()
    {
        static struct sockaddr_in st_addr = { 0 };
        socklen_t addrlen = sizeof(st_addr);

        if (getpeername(_sd, (struct sockaddr *)&st_addr, &addrlen) == -1)
            this->setstate(failbit);
        return (st_addr);
    }
    /**
     * @brief Get info about local loopback
     */
    const struct sockaddr_in &localinfo()
    {
        static struct sockaddr_in st_addr;
        socklen_t addrlen = sizeof(st_addr);
        Socket local;

        local.connect(0, INADDR_LOOPBACK);
        if (getsockname(local._sd, (struct sockaddr *)&st_addr, &addrlen) == -1)
            this->setstate(failbit);
        return (st_addr);
    }

private:
    /**
     * @brief Convert IPv4 adrress from text to binary form
     * @param addrstr dot-separated IPv4 address string
     * @param buffer in_addr_t buffer to fill with the binary address
     */
    static bool strToAddr(const char *addrstr, in_addr_t *buffer)
    {
        struct sockaddr_in st_addr = { 0 };
        if (inet_pton(AF_INET, addrstr, &(st_addr.sin_addr)) == -1)
            return (false);
        *buffer = st_addr.sin_addr.s_addr;
        return (true);
    }
    
    /**
     * @brief Convert binary address to IPv4 text address
     * @param addr binary IPv4 address
     */
    static const char *addrToStr(in_addr_t addr)
    {
        static char str[INET_ADDRSTRLEN] = { 0 };
        struct in_addr addr_st = {
            .s_addr = addr
        };

        return (inet_ntop(AF_INET, &addr_st, str, sizeof(str)));
    }
};