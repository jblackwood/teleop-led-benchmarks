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
#include "ThreadPool.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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
        AcceptorOpened,
        WebSocketConnected,
        Canceled
    };

    struct IOResult {
        IOResultType type;
        std::optional<tcp::acceptor> acceptor;
        std::optional<websocket::stream<tcp::socket>> ws;
    };

    IOResult openAcceptor(asio::io_context& ioc) {
        try {
            auto const address = asio::ip::make_address("127.0.0.1");
            auto const port = static_cast<unsigned short>(9002);
            return IOResult{
                .type = IOResultType::AcceptorOpened,
                .acceptor = tcp::acceptor{
                    ioc, {address, port}}};
        } catch (beast::system_error const& se) {
            std::cerr << "Error opening acceptor: " << se.code().message() << std::endl;
            std::abort();

        } catch (std::exception const& e) {
            std::cerr << "Error with acceptor: " << e.what() << std::endl;
            std::abort();
        }
    }

    IOResult waitForWebsocketConnection(
        asio::io_context& ioc,
        tcp::acceptor& acceptor) {
        try {
            // This will receive the new connection
            tcp::socket socket{ioc};
            
            // Block until we get a connection
            std::cout<< "socket accept call started"<<std::endl;
            acceptor.async_accept(
                socket,
                 [](boost::system::error_code ec) {
                if (!ec) {
                    std::cout << "Client connected!\n";
                } else {
                    std::cerr << "Accept failed: " << ec.message() << "\n";
                }
            }
            );
            ioc.run_one();
            
            // If app is closed run_one will return without assigning a socket
            // so need to check for that case.
            std::cout<< "socket accept call done"<<std::endl;
            if (!socket.is_open()){
                std::cout << "Socket not open" << std::endl;
                return IOResult{.type = IOResultType::Canceled};
            }
            
            // Construct the stream by moving in the socket
            websocket::stream<tcp::socket> ws{std::move(socket)};

            // Set a decorator to change the Server of the handshake
            ws.set_option(websocket::stream_base::decorator(
                [](websocket::response_type& res) {
                    res.set(http::field::server,
                            std::string(BOOST_BEAST_VERSION_STRING) +
                                " websocket-server-sync");
                }));

            // Accept the websocket handshake
            ws.accept();
            return IOResult{
                .type = IOResultType::WebSocketConnected,
                .ws = std::move(ws)};
        } catch (beast::system_error const& se) {
            std::cerr << "Error: " << se.code().message() << std::endl;
            std::abort();

        } catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            std::abort();
        }
    }

    class App {
    public:
        App() : showButton1_{true},
                showButton2_{false},
                threadPool_{1},
                ioc_{1} {
            pendingIOResult_ = threadPool_.enqueue(openAcceptor, std::ref(ioc_));
        };
        ~App() {
            std::cout<<"App destructor called" << std::endl;
            ioc_.stop();
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

        void processIOResult() {
            if (!pendingIOResult_.has_value()) {
                return;
            }
            auto& fut = pendingIOResult_.value();
            if (fut.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                return;
            }

            // Has a ready IOResult so process it
            auto res = fut.get();
            pendingIOResult_.reset();
            switch (res.type) {
            case IOResultType::AcceptorOpened:
                acceptor_ = std::move(res.acceptor);
                pendingIOResult_ = threadPool_.enqueue(waitForWebsocketConnection, std::ref(ioc_), std::ref(acceptor_.value()));
                break;
            case IOResultType::WebSocketConnected:
                throw std::runtime_error("Not implemented yet");
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
        ThreadPool threadPool_;
        asio::io_context ioc_;
        bool showButton1_;
        bool showButton2_;
        std::vector<UIEvent> uiEventsToProcess_;
        std::optional<std::future<IOResult>> pendingIOResult_;
        std::optional<tcp::acceptor> acceptor_;
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
            app.processIOResult();
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
        std::cout<< "Exited loop" << std::endl;

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }
}