#include <GLFW/glfw3.h>
#include <stdio.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <future>
#include <iostream>
#include <optional>
#include <queue>
#include <string>
#include <vector>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "readerwriterqueue.h"

namespace fast_led_teleop::desktop {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace websocket = beast::websocket;
    namespace asio = boost::asio;
    using tcp = boost::asio::ip::tcp;

    constexpr float windowXPadding = 50.0f;

    enum class UIEventType {
        Button1Click,
        Button2Click
    };

    struct UIEvent {
        UIEventType type;
    };

    enum class IOResultType {
        WebSocketConnected,
        Canceled
    };

    struct IOResult {
        IOResultType type;
        std::unique_ptr<websocket::stream<tcp::socket>> ws = nullptr;
    };

    void async_waitForWebsocketConnection(asio::io_context& ioc, moodycamel::ReaderWriterQueue<IOResult>& ioResults) {
        auto const address = asio::ip::make_address("127.0.0.1");
        auto const port = static_cast<unsigned short>(9002);
        tcp::endpoint endpoint{address, port};
        auto acceptor = std::make_unique<tcp::acceptor>(ioc, endpoint);
        std::cout << "async accepting" << std::endl;
        acceptor->async_accept(
            ioc,
            [&ioResults,
             acceptor = std::move(acceptor)](boost::system::error_code ec, tcp::socket socket) {
                if (ec) {
                    std::cerr << "Accept failed: " << ec.message() << "\n";
                    return;
                }
                std::cout << "Client connected!" << std::endl;
                // acceptor->close();
                if (!socket.is_open()) {
                    std::cout << "Socket not open" << std::endl;
                    ioResults.emplace(IOResult{.type = IOResultType::Canceled});
                    return;
                }

                // Construct the stream by moving in the socket
                auto ws = std::make_unique<websocket::stream<tcp::socket>>(std::move(socket));

                // Set a decorator to change the Server of the handshake
                ws->set_option(websocket::stream_base::decorator(
                    [](websocket::response_type& res) {
                        res.set(http::field::server,
                                std::string(BOOST_BEAST_VERSION_STRING) +
                                    " websocket-server-sync");
                    }));

                // Accept the websocket handshake. This will block the io thread, but should be fast
                try {
                    ws->accept();
                } catch (beast::system_error const& se) {
                    std::cerr << "Error opening acceptor: " << se.code().message() << std::endl;
                    std::abort();

                } catch (std::exception const& e) {
                    std::cerr << "Error with acceptor: " << e.what() << std::endl;
                    std::abort();
                }
                auto res = IOResult{
                    .type = IOResultType::WebSocketConnected,
                    .ws = std::move(ws)};
                ioResults.enqueue(std::move(res));
                return;
            });
        std::cout << "done async accepting" << std::endl;
    }

    class App {
    public:
        App() : ioc_{1},
                ioResults_{5},
                showButton1_{true},
                showButton2_{false},
                timer_{ioc_, std::chrono::seconds(5)} {
            async_waitForWebsocketConnection(ioc_, ioResults_);
            timer_.async_wait([](const boost::system::error_code& ec) {
                if (!ec) {
                    std::cout << "Timer expired 1!" << std::endl;
                } else {
                    std::cout << "Timer expired 2!" << std::endl;
                }
            });
            std::cout << "starting io thread" << std::endl;
            ioThread_ = std::thread([this]() { ioc_.run(); });
        };
        ~App() {
            std::cout << "App destructor called" << std::endl;
            ioc_.stop();
            ioThread_.join();
            std::cout << "App destructor done" << std::endl;
        };
        App(const App& other) = delete;
        App& operator=(const App& other) = delete;
        App(App&& other) = delete;
        App& operator=(App&& other) = delete;

        void processUiEvents() {
            for (auto& e : uiEventsToProcess_) {
                switch (e.type) {
                case UIEventType::Button1Click:
                    showButton1_ = false;
                    showButton2_ = true;
                    break;
                case UIEventType::Button2Click:
                    showButton1_ = true;
                    showButton2_ = false;
                }
            }
            uiEventsToProcess_.clear();
        };

        void processIOResults() {
            static IOResult res;
            while (ioResults_.try_dequeue(res)) {
                switch (res.type) {
                case IOResultType::WebSocketConnected:
                    std::cout << "Websocket connected io result" << std::endl;
                    ws_ = std::move(res.ws);
                    ws_->async_read(buffer_,[](boost::system::error_code ec, std::size_t num_bytes){
                        std::cout << "ec received" << ec << std::endl;
                        std::cout <<"bytes received" << num_bytes << std::endl;
                    });
                    break;

                case IOResultType::Canceled:
                    std::cout << "IO cancelled" << std::endl;
                    break;
                }
            }
        };

        void render() {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImVec2 windowPos(windowXPadding, viewport->WorkPos.y);
            ImGui::SetNextWindowPos(windowPos);
            ImVec2 windowSize((viewport->WorkSize.x) - 2 * windowXPadding,
                              viewport->WorkSize.y);
            ImGui::SetNextWindowSize(windowSize);
            ImGui::Begin("Hello, ImGui!", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoNavFocus);
            ImGui::Text("This is a simple window.");
            ImGui::Text("This is 2nd text.");
            if (showButton1_) {
                if (ImGui::Button("Button 1")) {
                    uiEventsToProcess_.push_back(UIEvent{.type = UIEventType::Button1Click});
                };
            } else if (showButton2_) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
                if (ImGui::Button("Button 2")) {
                    uiEventsToProcess_.push_back(UIEvent{.type = UIEventType::Button2Click});
                };
                ImGui::PopStyleColor();
            }
            ImGui::End();
        };

    private:
        std::thread ioThread_;
        asio::io_context ioc_;
        moodycamel::ReaderWriterQueue<IOResult> ioResults_;
        std::unique_ptr<websocket::stream<tcp::socket>> ws_;
        beast::flat_buffer buffer_;
        bool showButton1_;
        bool showButton2_;
        std::vector<UIEvent> uiEventsToProcess_;
        asio::steady_timer timer_;
    };

    int runApp(const std::atomic<bool>& stopFlag) {
        App app{};

        glfwInit();
        GLFWwindow* window = glfwCreateWindow(720, 720, "ImGui Example", NULL, NULL);
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        while (!glfwWindowShouldClose(window) && !stopFlag.load(std::memory_order_relaxed)) {
            // Render
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Core app logic
            app.processUiEvents();
            app.processIOResults();
            app.render();

            // ImGui::ShowDemoWindow();
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
        }
        std::cout << "Exited loop" << std::endl;

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        std::cout << "calling terminate" << std::endl;
        glfwTerminate();
        std::cout << "terminate done" << std::endl;
        return 0;
    }
}