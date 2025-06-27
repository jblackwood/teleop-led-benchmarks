#include "app.hpp"

#include <GLFW/glfw3.h>
#include <stdio.h>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <future>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace teleop_led_benchmarks
{
namespace desktop
{


namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using chrono_time_point = std::chrono::steady_clock::time_point;
using tcp = boost::asio::ip::tcp;


constexpr float WINDOW_X_PADDING = 50.0f;


const static std::string EXPECTED_MSG = "received";


enum class UIEventType
{
    SEND_BUTTON_CLICK,
};


struct UIEvent
{
    UIEventType type;
};


enum class IOResultType
{
    WS_CONNECTED,
    WS_MSG_RECEIVED,
    TCP_CONNECTED,
    TCP_MESSAGE_RECEIVED,
    CANCELLED
};


struct IOResult
{
    IOResultType type;
    std::unique_ptr<websocket::stream<tcp::socket>> ws = nullptr;
    std::unique_ptr<tcp::socket> tcpSock = nullptr;
};


constexpr std::array<std::string_view, 2> CONNECTION_TYPE_STRINGS = {"WebSocket", "CustomTcp"};


void asyncWaitForTcpConnection(asio::io_context& ioc, std::vector<IOResult>& ioResults)
{
    auto const address = asio::ip::make_address("0.0.0.0");
    auto const port = static_cast<unsigned short>(9003);
    tcp::endpoint endpoint{address, port};
    auto acceptor = std::make_unique<tcp::acceptor>(ioc, endpoint);
    std::cout << "async tcp accepting" << std::endl;
    acceptor->async_accept(
        ioc,
        [&ioResults, acceptor = std::move(acceptor)](boost::system::error_code ec,
            tcp::socket socket)
        {
            std::cout << "tcp async accept handler" << std::endl;
            if (ec)
            {
                std::cerr << "accept failed: " << ec.message() << "\n";
                return;
            }
            std::cout << "tcp client connected!" << std::endl;
            acceptor->close();
            if (!socket.is_open())
            {
                std::cout << "Socket not open" << std::endl;
                ioResults.emplace_back(IOResult{.type = IOResultType::CANCELLED});
                return;
            }
            auto res = IOResult{.type = IOResultType::TCP_CONNECTED,
                .tcpSock = std::make_unique<tcp::socket>(std::move(socket))};
            ioResults.push_back(std::move(res));
            return;
        });
    std::cout << "done async accepting" << std::endl;
}


void asyncWaitForWebsocketConnection(asio::io_context& ioc, std::vector<IOResult>& ioResults)
{
    auto const address = asio::ip::make_address("0.0.0.0");
    auto const port = static_cast<unsigned short>(9002);
    tcp::endpoint endpoint{address, port};
    auto acceptor = std::make_unique<tcp::acceptor>(ioc, endpoint);
    std::cout << "async websocket accepting" << std::endl;
    acceptor->async_accept(
        ioc,
        [&ioResults, acceptor = std::move(acceptor)](boost::system::error_code ec,
            tcp::socket socket)
        {
            std::cout << "Async accept handler" << std::endl;
            if (ec)
            {
                std::cerr << "Accept failed: " << ec.message() << "\n";
                return;
            }
            std::cout << "Client connected!" << std::endl;
            acceptor->close();
            if (!socket.is_open())
            {
                std::cout << "Socket not open" << std::endl;
                ioResults.emplace_back(IOResult{.type = IOResultType::CANCELLED});
                return;
            }

            // Construct the stream by moving in the socket
            auto ws = std::make_unique<websocket::stream<tcp::socket>>(std::move(socket));

            // Set a decorator to change the Server of the handshake
            ws->set_option(websocket::stream_base::decorator(
                [](websocket::response_type& res)
                {
                    res.set(http::field::server,
                        std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-sync");
                }));

            // Accept the websocket handshake. This will block the io thread, but should be fast
            try
            {
                ws->accept();
            }
            catch (beast::system_error const& se)
            {
                std::cerr << "Error opening acceptor: " << se.code().message() << std::endl;
                std::abort();
            }
            catch (std::exception const& e)
            {
                std::cerr << "Error with acceptor: " << e.what() << std::endl;
                std::abort();
            }
            auto res = IOResult{.type = IOResultType::WS_CONNECTED, .ws = std::move(ws)};
            ioResults.push_back(std::move(res));
            return;
        });
    std::cout << "done async accepting" << std::endl;
}


struct AppState
{
    asio::io_context ioc;
    std::vector<UIEvent> uiEventsToProcess;
    std::vector<IOResult> ioResults;
    ConnectionType connType;

    std::unique_ptr<tcp::socket> tcpSock;
    std::string tcpReadBuf;

    std::unique_ptr<websocket::stream<tcp::socket>> ws;
    beast::flat_buffer wsReadBuffer;

    bool isSendingBlinkCommand;
    chrono_time_point timeSendBlinkCommand;
    std::chrono::duration<double, std::milli> blinkLatency;

    AppState(ConnectionType initialConnType)
        : ioc{1},
          connType{initialConnType},
          tcpReadBuf(EXPECTED_MSG.size(), '\0'),  // always expect "received" for now
          isSendingBlinkCommand{false},
          blinkLatency{0.0f}

    {
        std::cout << "Creating app state" << std::endl;
        switch (connType)
        {
            case ConnectionType::WEB_SOCKET:
            {
                asyncWaitForWebsocketConnection(ioc, ioResults);
                break;
            }
            case ConnectionType::CUSTOM_TCP:
            {
                asyncWaitForTcpConnection(ioc, ioResults);
                break;
            }
        }

        std::cout << "Done creating app state" << std::endl;
    };

    ~AppState() = default;
    AppState(const AppState& other) = delete;
    AppState& operator=(const AppState& other) = delete;
    AppState(AppState&& other) = delete;
    AppState& operator=(AppState&& other) = delete;
};


void handleSendButtonClick(AppState& s)
{
    s.isSendingBlinkCommand = true;
    s.timeSendBlinkCommand = std::chrono::steady_clock::now();
    switch (s.connType)
    {
        case ConnectionType::WEB_SOCKET:
        {
            s.ws->async_write(boost::asio::buffer("button clicked\n"),
                [](boost::system::error_code ec, std::size_t bytesTransferred)
                {
                    (void) bytesTransferred;
                    if (ec)
                    {
                        std::cout << "Error writing to websocket" << std::endl;
                        return;
                    }
                });
            break;
        }
        case ConnectionType::CUSTOM_TCP:
        {
            std::cout << "sending tcp click" << std::endl;
            asio::async_write(*s.tcpSock, asio::buffer("button clicked\n"),
                [](boost::system::error_code ec, std::size_t bytesTransferred)
                {
                    (void) bytesTransferred;
                    std::cout << "done writing to tcp" << std::endl;
                    if (ec)
                    {
                        std::cout << "Error writing to tcp" << std::endl;
                        return;
                    }
                });
            break;
        }
    }
}


void processUiEvents(AppState& s)
{
    for (auto& e : s.uiEventsToProcess)
    {
        switch (e.type)
        {
            case UIEventType::SEND_BUTTON_CLICK:
            {
                handleSendButtonClick(s);
                break;
            }
        }
    }
    s.uiEventsToProcess.clear();
};


void wsAsyncRead(AppState& s)
{
    s.ws->async_read(
        s.wsReadBuffer,
        [&s](boost::system::error_code ec, std::size_t numBytes) mutable
        {
            std::cout << "ws bytes received " << numBytes << "\n";
            if (ec == websocket::error::closed)
            {
                std::cout << "Websocket closed" << std::endl;
                return;
            }

            if (ec)
            {
                std::cout << "ec received " << ec << std::endl;
                return;
            }
            if (numBytes == 0)
            {
                return;
            }
            IOResult res{};
            res.type = IOResultType::WS_MSG_RECEIVED;
            s.ioResults.push_back(std::move(res));
            return;
        });
};

void tcpAsyncRead(AppState& s)
{
    asio::async_read(
        *s.tcpSock,
        asio::buffer(s.tcpReadBuf),
        [&s](const boost::system::error_code& ec, std::size_t bytesTransferred)
        {
            std::cout << "tcp received bytes transferred " << bytesTransferred
                      << std::endl;
            (void) bytesTransferred;
            if (ec)
            {
                std::cout << "tcp received failed" << std::endl;
                return;
            }

            IOResult res{};
            res.type = IOResultType::TCP_MESSAGE_RECEIVED;
            s.ioResults.push_back(std::move(res));
        });
}


void processIOResults(AppState& s)
{
    for (auto& res : s.ioResults)
    {
        switch (res.type)
        {
            case IOResultType::WS_CONNECTED:
            {
                std::cout << "WsConnected" << std::endl;
                s.ws = std::move(res.ws);
                wsAsyncRead(s);
                break;
            }

            case IOResultType::WS_MSG_RECEIVED:
            {
                std::string_view msg{static_cast<const char*>(s.wsReadBuffer.data().data()),
                    s.wsReadBuffer.size()};
                std::cout << "WsMsgReceived " << msg << std::endl;
                s.wsReadBuffer.consume(s.wsReadBuffer.size());
                s.isSendingBlinkCommand = false;
                s.blinkLatency = std::chrono::steady_clock::now() - s.timeSendBlinkCommand;
                wsAsyncRead(s);
                break;
            }

            case IOResultType::TCP_CONNECTED:
            {
                std::cout << "TcpConnected" << std::endl;
                s.tcpSock = std::move(res.tcpSock);
                tcpAsyncRead(s);
                break;
            }

            case IOResultType::TCP_MESSAGE_RECEIVED:
            {
                std::cout << "TcpMessageReceived " << s.tcpReadBuf << std::endl;
                s.isSendingBlinkCommand = false;
                s.blinkLatency = std::chrono::steady_clock::now() - s.timeSendBlinkCommand;
                tcpAsyncRead(s);
                break;
            }

            case IOResultType::CANCELLED:
            {
                std::cout << "IO cancelled" << std::endl;
                break;
            }
        }
    }
    s.ioResults.clear();
};


void render(AppState& s)
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 windowPos(WINDOW_X_PADDING, viewport->WorkPos.y);
    ImGui::SetNextWindowPos(windowPos);
    ImVec2 windowSize((viewport->WorkSize.x) - 2 * WINDOW_X_PADDING, viewport->WorkSize.y);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::Begin("Full Screen Window", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus);

    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    auto idxConnType = static_cast<size_t>(s.connType);
    ImGui::Text("Connection type: %s", CONNECTION_TYPE_STRINGS[idxConnType].data());
    if (s.ws == nullptr && s.tcpSock == nullptr)
    {
        ImGui::Text("Waiting for esp32 to connect");
    }
    else
    {
        ImGui::Text("esp32 connected");
        ImGui::BeginDisabled(s.isSendingBlinkCommand);
        if (ImGui::Button("Send blink command"))
        {
            s.uiEventsToProcess.push_back(UIEvent{.type = UIEventType::SEND_BUTTON_CLICK});
        };
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::Text("last blink latency %.2f ms", s.blinkLatency.count());
    }
    ImGui::End();
};


void runAppLoop(AppState& s, GLFWwindow* window, const std::atomic<bool>& stopFlag)
{
    if (glfwWindowShouldClose(window))
    {
        std::cout << "stopping due to closed window" << std::endl;
        s.ioc.stop();
        return;
    }
    if (stopFlag.load(std::memory_order_relaxed))
    {
        std::cout << "stopping due to stop flag" << std::endl;
        s.ioc.stop();
        return;
    }

    // ImGUI new frame setup
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Core app logic
    processUiEvents(s);
    processIOResults(s);
    render(s);
    // ImGui::ShowDemoWindow(); // if wanting to see ui examples

    // ImGui render
    ImGui::Render();
    int displayW, displayH;
    glfwGetFramebufferSize(window, &displayW, &displayH);
    glViewport(0, 0, displayW, displayH);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    /**
     * glfwSwapInterval(1) above enables vsync.
     * That means glfwSwapBuffers() blocks until the display's
     * next refresh (e.g., 1/60s on a 60Hz screen).
     */
    glfwSwapBuffers(window);

    // Need to capture window pointer by value because arg value
    // pops off the stack frame causing bugs.
    asio::post(s.ioc, [&s, window, &stopFlag]()
        { runAppLoop(s, window, stopFlag); });
}


int runApp(const std::atomic<bool>& stopFlag, const ConnectionType connType)
{
    AppState s{connType};
    std::cout << "io results size " << s.ioResults.size() << std::endl;
    glfwInit();

    // These hints MUST come before glfwCreateWindow
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // Needed on macOS

    GLFWwindow* window = glfwCreateWindow(720, 720, "Teleop LED Benchmarking", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
    // auto work_guard = boost::asio::make_work_guard(s.ioc);
    runAppLoop(s, window, stopFlag);
    s.ioc.run();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    std::cout << "calling terminate" << std::endl;
    glfwTerminate();
    std::cout << "terminate done" << std::endl;
    return 0;
}


}  // namespace desktop
}  // namespace teleop_led_benchmarks