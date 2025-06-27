#include "tcp_client.hpp"
#include "esp_log.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

static const char* TAG = "tcpclient";

TcpClient::TcpClient(const std::string& host, uint16_t port)
    : host_ip_(host), port_(port), sock_(-1) {}

TcpClient::~TcpClient()
{
    disconnect();
}

int TcpClient::connectToServer()
{
    struct sockaddr_in dest_addr{};
    inet_pton(AF_INET, host_ip_.c_str(), &dest_addr.sin_addr);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port_);

    sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock_ < 0)
    {
        ESP_LOGE(TAG, "failed to create socket");
        return -1;
    }

    if (connect(sock_, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr)) != 0)
    {
        close(sock_);
        sock_ = -1;
        ESP_LOGE(TAG, "Failed to connect to server");
        return -1;
    }

    int flag = 1;
    int result = setsockopt(sock_, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    if (result < 0)
    {
        ESP_LOGE("TCP", "Failed to disable Nagle's algorithm");
    }
    else
    {
        ESP_LOGI("TCP", "Nagle's algorithm disabled");
    }

    return 0;
}

int TcpClient::sendData(const std::string& data)
{
    if (sock_ < 0)
    {
        ESP_LOGE(TAG, "Socket not connected");
        return -1;
    }

    if (send(sock_, data.c_str(), data.size(), 0) < 0)
    {
        ESP_LOGE(TAG, "Failed to send data");
        return -1;
    }
    return 0;
}

// Caller must ensure string is sized correctly)
int TcpClient::receiveData(std::string& data)
{
    if (sock_ < 0)
    {
        ESP_LOGE(TAG, "Socket not connected");
        return -1;
    }

    int len = recv(sock_, data.data(), data.size() + 1, 0);
    if (len < 0)
    {
        ESP_LOGE(TAG, "Failed to receive data");
        return -1;
    }

    return 0;
}

void TcpClient::disconnect()
{
    if (sock_ != -1)
    {
        shutdown(sock_, 0);
        close(sock_);
        sock_ = -1;
    }
}
