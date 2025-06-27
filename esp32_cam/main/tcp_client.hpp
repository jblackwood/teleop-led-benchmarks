#pragma once
#include <unistd.h>

#include <string>


class TcpClient
{
   public:
    TcpClient(const std::string& host, uint16_t port);
    ~TcpClient();
    int connectToServer();
    int sendData(const std::string& data);
    int receiveData(std::string& data);
    void disconnect();

   private:
    std::string host_ip_;
    uint16_t port_;
    int sock_;
};
