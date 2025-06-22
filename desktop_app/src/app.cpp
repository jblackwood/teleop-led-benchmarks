#include <GLFW/glfw3.h>
#include <stdio.h>

#include <queue>
#include <string>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace fast_led_teleop::desktop {
    constexpr float windowXPadding = 50.0f;

    struct AppState {
        bool button1;
        bool button2;
    };

    enum class EventType {
        Button1Click,
        Button2Click
    };

    struct Event {
        EventType type;
    };

    void renderApp(const AppState& s, std::vector<Event>& newUiEvents) {
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
        if (s.button1) {
            if (ImGui::Button("Button 1")) {
                newUiEvents.push_back(Event{.type = EventType::Button1Click});
            };
        } else if (s.button2) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
            if (ImGui::Button("Button 2")) {
                newUiEvents.push_back(Event{.type = EventType::Button2Click});
            };
            ImGui::PopStyleColor();
        }
        ImGui::End();
    }

    void updateState(AppState& s, const Event& e) {
        switch (e.type) {
        case EventType::Button1Click:
            s.button1 = false;
            s.button2 = true;
            break;
        case EventType::Button2Click:
            s.button1 = true;
            s.button2 = false;
        }
    }

    int runApp() {
        AppState s{.button1 = true, .button2 = false};
        std::vector<Event> newUiEvents;

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

        while (!glfwWindowShouldClose(window)) {
            for (auto& e : newUiEvents) {
                updateState(s, e);
            }
            newUiEvents.clear();

            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            renderApp(s, newUiEvents);
            // ImGui::ShowDemoWindow();
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            /**
             * glfwSwapInterval(1) above enables vsync.
             * => glfwSwapBuffers() blocks until the display's next refresh (e.g., 1/60s on a 60Hz screen)
             */
            glfwSwapBuffers(window);
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }
}