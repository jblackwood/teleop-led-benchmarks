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


constexpr float windowXPadding = 50.0f;


const static std::string expectedMsg = "received";


enum class UIEventType
{
    SendButtonClick,
};


struct UIEvent
{
    UIEventType type;
};


enum class IOResultType
{
    WsConnected,
    WsMsgReceived,
    TcpConnected,
    TcpMessageReceived,
    Cancelled
};


struct IOResult
{
    IOResultType type;
    std::unique_ptr<websocket::stream<tcp::socket>> ws = nullptr;
    std::unique_ptr<tcp::socket> tcp_sock = nullptr;
};


constexpr std::array<std::string_view, 2> connectionTypeStrings = {"WebSocket", "CustomTcp"};


void async_waitForTcpConnection(asio::io_context& ioc, std::vector<IOResult>& ioResults)
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
                ioResults.emplace_back(IOResult{.type = IOResultType::Cancelled});
                return;
            }
            auto res = IOResult{.type = IOResultType::TcpConnected,
                .tcp_sock = std::make_unique<tcp::socket>(std::move(socket))};
            ioResults.push_back(std::move(res));
            return;
        });
    std::cout << "done async accepting" << std::endl;
}


void async_waitForWebsocketConnection(asio::io_context& ioc, std::vector<IOResult>& ioResults)
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
                ioResults.emplace_back(IOResult{.type = IOResultType::Cancelled});
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
            auto res = IOResult{.type = IOResultType::WsConnected, .ws = std::move(ws)};
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

    std::unique_ptr<tcp::socket> tcp_sock;
    std::string tcp_read_buf;

    std::unique_ptr<websocket::stream<tcp::socket>> ws;
    beast::flat_buffer ws_read_buffer;

    bool isSendingBlinkCommand;
    chrono_time_point timeSendBlinkCommand;
    std::chrono::duration<double, std::milli> blinkLatency;

    AppState(ConnectionType initialConnType)
        : ioc{1},
          connType{initialConnType},
          tcp_read_buf(expectedMsg.size(), '\0'),  // always expect "received" for now
          isSendingBlinkCommand{false},
          blinkLatency{0.0f}

    {
        std::cout << "Creating app state" << std::endl;
        switch (connType)
        {
            case ConnectionType::WebSocket:
            {
                async_waitForWebsocketConnection(ioc, ioResults);
                break;
            }
            case ConnectionType::CustomTcp:
            {
                async_waitForTcpConnection(ioc, ioResults);
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
        case ConnectionType::WebSocket:
        {
            s.ws->async_write(boost::asio::buffer("button clicked\n"),
                [](boost::system::error_code ec, std::size_t bytes_transferred)
                {
                    (void) bytes_transferred;
                    if (ec)
                    {
                        std::cout << "Error writing to websocket" << std::endl;
                        return;
                    }
                });
            break;
        }
        case ConnectionType::CustomTcp:
        {
            std::cout << "sending tcp click" << std::endl;
            asio::async_write(*s.tcp_sock, asio::buffer("button clicked\n"),
                [](boost::system::error_code ec, std::size_t bytes_transferred)
                {
                    (void) bytes_transferred;
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
            case UIEventType::SendButtonClick:
            {
                handleSendButtonClick(s);
                break;
            }
        }
    }
    s.uiEventsToProcess.clear();
};


void ws_async_read(AppState& s)
{
    s.ws->async_read(
        s.ws_read_buffer,
        [&s](boost::system::error_code ec, std::size_t num_bytes) mutable
        {
            std::cout << "ws bytes received " << num_bytes << "\n";
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
            if (num_bytes == 0)
            {
                return;
            }
            IOResult res{};
            res.type = IOResultType::WsMsgReceived;
            s.ioResults.push_back(std::move(res));
            return;
        });
};

void tcpAsyncRead(AppState& s)
{
    asio::async_read(
        *s.tcp_sock,
        asio::buffer(s.tcp_read_buf),
        [&s](const boost::system::error_code& ec, std::size_t bytes_transferred)
        {
            std::cout << "tcp received bytes transferred " << bytes_transferred
                      << std::endl;
            (void) bytes_transferred;
            if (ec)
            {
                std::cout << "tcp received failed" << std::endl;
                return;
            }

            IOResult res{};
            res.type = IOResultType::TcpMessageReceived;
            s.ioResults.push_back(std::move(res));
        });
}


void processIOResults(AppState& s)
{
    for (auto& res : s.ioResults)
    {
        switch (res.type)
        {
            case IOResultType::WsConnected:
            {
                std::cout << "WsConnected" << std::endl;
                s.ws = std::move(res.ws);
                ws_async_read(s);
                break;
            }

            case IOResultType::WsMsgReceived:
            {
                std::string_view msg{static_cast<const char*>(s.ws_read_buffer.data().data()),
                    s.ws_read_buffer.size()};
                std::cout << "WsMsgReceived " << msg << std::endl;
                s.ws_read_buffer.consume(s.ws_read_buffer.size());
                s.isSendingBlinkCommand = false;
                s.blinkLatency = std::chrono::steady_clock::now() - s.timeSendBlinkCommand;
                ws_async_read(s);
                break;
            }

            case IOResultType::TcpConnected:
            {
                std::cout << "TcpConnected" << std::endl;
                s.tcp_sock = std::move(res.tcp_sock);
                tcpAsyncRead(s);
                break;
            }

            case IOResultType::TcpMessageReceived:
            {
                std::cout << "TcpMessageReceived " << s.tcp_read_buf << std::endl;
                s.isSendingBlinkCommand = false;
                s.blinkLatency = std::chrono::steady_clock::now() - s.timeSendBlinkCommand;
                tcpAsyncRead(s);
                break;
            }

            case IOResultType::Cancelled:
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
    ImVec2 windowPos(windowXPadding, viewport->WorkPos.y);
    ImGui::SetNextWindowPos(windowPos);
    ImVec2 windowSize((viewport->WorkSize.x) - 2 * windowXPadding, viewport->WorkSize.y);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::Begin("Full Screen Window", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus);

    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    auto idxConnType = static_cast<size_t>(s.connType);
    ImGui::Text("Connection type: %s", connectionTypeStrings[idxConnType].data());
    if (s.ws == nullptr && s.tcp_sock == nullptr)
    {
        ImGui::Text("Waiting for esp32 to connect");
    }
    else
    {
        ImGui::Text("esp32 connected");
        ImGui::BeginDisabled(s.isSendingBlinkCommand);
        if (ImGui::Button("Send blink command"))
        {
            s.uiEventsToProcess.push_back(UIEvent{.type = UIEventType::SendButtonClick});
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
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
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