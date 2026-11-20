// A4 GLFW viewer -- provided framework; assignment TODOs are in lsystem.cpp/.hpp.
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Include our GL loader first. On Windows it also provides windows.h/APIENTRY.
#include "lsystem.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifdef _WIN32
// Use a small native popup-menu adapter on Windows only;
// the window, OpenGL context and normal event handling still belong to GLFW.
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_NATIVE_INCLUDE_NONE
#include <GLFW/glfw3native.h>
#endif

namespace fs = std::filesystem;

const int MENU_EXIT = 1;
const int MENU_PREVITER = 2;
const int MENU_NEXTITER = 3;
const int MENU_REPARSE = 4;
const int MENU_OBJBASE = 64;

GLFWwindow* window = nullptr;
bool glfwReady = false;
bool redrawRequested = true;
bool popupRequested = false;
int width = 800, height = 600; // Framebuffer pixels, NOT logical window size.
std::unique_ptr<LSystem> lsystem;
std::vector<std::string> modelFilenames;
unsigned int iter = 0;
std::string lastFilename;
int lastFilenameIdx = -1;

void menu(int cmd);
void display();
void updateTitle();
void showContextMenu();
void cleanup();

void printHelp() {
    std::cout
        << "\nA4 - L-Systems / GLFW\n"
        << "  Up / Down   : previous / next input file\n"
        << "  Left / Right: previous / next iteration\n"
        << "  Space or R  : reload current input file\n"
        << "  Escape      : exit\n"
        << "  H or F1     : show these controls\n"
        << "  Drop a .txt file onto the window to load it.\n";
#ifdef _WIN32
    std::cout << "  Right click : model / iteration / reload menu\n";
#else
    std::cout << "  Right click : print input-file list (use Up/Down to select)\n";
#endif
    std::cout << std::endl;
}

// Resources are found from the project root when launched in Visual Studio.
// They are also copied next to the exe, so Explorer launch works without PATH
// settings or a developer-specific .vcxproj.user file.
void locateResources(const char* argv0) {
    std::vector<fs::path> candidates{fs::current_path()};
#ifdef _WIN32
    std::vector<wchar_t> pathBuffer(32768);
    DWORD count = GetModuleFileNameW(nullptr, pathBuffer.data(),
                                   static_cast<DWORD>(pathBuffer.size()));
    if (count > 0 && count < pathBuffer.size()) {
        candidates.push_back(fs::path(std::wstring(pathBuffer.data(), count)).parent_path());
    }
#endif
    if (argv0 && *argv0) {
        std::error_code ec;
        fs::path executable = fs::absolute(fs::path(argv0), ec);
        if (!ec) candidates.push_back(executable.parent_path());
    }
    for (auto base : candidates) {
        // Also permit launch from a nested build directory.
        for (int level = 0; level < 5 && !base.empty(); ++level) {
            if (fs::is_regular_file(base / "shaders/v.glsl") &&
                fs::is_regular_file(base / "shaders/f.glsl") &&
                fs::is_directory(base / "models")) {
                fs::current_path(base);
                return;
            }
            const fs::path parent = base.parent_path();
            if (parent == base) break;
            base = parent;
        }
    }
    throw std::runtime_error("Cannot find models/ and shaders/. Run from the extracted project folder.");
}

void findModelFiles() {
    modelFilenames.clear();
    for (const auto& entry : fs::directory_iterator("models")) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
            modelFilenames.push_back(entry.path().generic_string());
    }
    std::sort(modelFilenames.begin(), modelFilenames.end());
    if (modelFilenames.empty())
        std::cerr << "No .txt files found in models/. You can drop a file on the window.\n";
}

void updateTitle() {
    if (!window) return;
    std::string title = "A4 - GLFW";
    if (!lastFilename.empty()) {
        title += " | " + fs::path(lastFilename).filename().u8string();
        title += " | Iteration " + std::to_string(iter);
    }
    title += " | Up/Down: model, Left/Right: iteration";
    glfwSetWindowTitle(window, title.c_str());
}

void loadModel(const std::string& path) {
    lsystem->parseFile(path);
    lastFilename = path;
    lastFilenameIdx = -1;
    const auto normalized = fs::absolute(fs::path(path)).lexically_normal();
    for (std::size_t i = 0; i < modelFilenames.size(); ++i) {
        if (fs::absolute(fs::path(modelFilenames[i])).lexically_normal() == normalized) {
            lastFilenameIdx = static_cast<int>(i);
            break;
        }
    }
    const unsigned int count = lsystem->getNumIter();
    iter = count > 0 ? count - 1 : 0;
    std::cout << "Loaded " << lastFilename << " | Iteration " << iter << std::endl;
    updateTitle();
    redrawRequested = true;
}

// All UI routes share the same LSystem operations and iteration numbering.
void menu(int cmd) {
    if (cmd == MENU_EXIT) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }
    if (!lsystem) return;
    try {
        if (cmd == MENU_PREVITER) {
            if (iter > 0) --iter;
        } else if (cmd == MENU_NEXTITER) {
            if (lsystem->getNumIter() == 0) return;
            if (iter + 1 >= lsystem->getNumIter()) lsystem->iterate();
            if (iter + 1 < lsystem->getNumIter()) ++iter;
        } else if (cmd == MENU_REPARSE) {
            if (!lastFilename.empty()) loadModel(lastFilename);
            return;
        } else if (cmd >= MENU_OBJBASE) {
            const std::size_t index = static_cast<std::size_t>(cmd - MENU_OBJBASE);
            if (index < modelFilenames.size()) loadModel(modelFilenames[index]);
            return;
        }
        std::cout << "Iteration " << iter << std::endl;
        updateTitle();
    } catch (const std::exception& e) {
        // Keep exceptions inside C++ rather than unwinding through a C callback.
        std::cerr << "Operation failed: " << e.what() << std::endl;
        if (lsystem->getNumIter() > 0)
            iter = std::min(iter, lsystem->getNumIter() - 1);
        else iter = 0;
    }
    redrawRequested = true;
}

void display() {
    if (!window || width <= 0 || height <= 0) return;
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    glm::mat3 proj(1.0f);
    proj[0][0] = glm::min(1.0f / aspect, 1.0f);
    proj[1][1] = glm::min(aspect, 1.0f);
    if (lsystem && iter < lsystem->getNumIter()) lsystem->drawIter(iter, proj);
    glfwSwapBuffers(window);
}

void errorCallback(int error, const char* description) {
    std::cerr << "GLFW error " << error << ": "
              << (description ? description : "Unknown error") << std::endl;
}

void framebufferSizeCallback(GLFWwindow*, int w, int h) {
    width = w;
    height = h;
    redrawRequested = true;
}

void refreshCallback(GLFWwindow*) { redrawRequested = true; }

void keyCallback(GLFWwindow*, int key, int, int action, int) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    if (key == GLFW_KEY_ESCAPE) { menu(MENU_EXIT); return; }
    if (key == GLFW_KEY_LEFT)  { menu(MENU_PREVITER); return; }
    if (key == GLFW_KEY_RIGHT) { menu(MENU_NEXTITER); return; }
    if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN) {
        const int count = static_cast<int>(modelFilenames.size());
        if (count == 0) return;
        int index = lastFilenameIdx;
        if (index < 0) index = key == GLFW_KEY_UP ? count - 1 : 0;
        else index = (index + (key == GLFW_KEY_UP ? -1 : 1) + count) % count;
        menu(MENU_OBJBASE + index);
        return;
    }
    if (action != GLFW_PRESS) return; // No repeated reparsing while Space is held.
    if (key == GLFW_KEY_SPACE || key == GLFW_KEY_R) menu(MENU_REPARSE);
    else if (key == GLFW_KEY_H || key == GLFW_KEY_F1) printHelp();
}

void mouseButtonCallback(GLFWwindow*, int button, int action, int) {
    // Defer the native modal menu until after GLFW finishes dispatching callbacks.
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        popupRequested = true;
}

void dropCallback(GLFWwindow*, int count, const char** paths) {
    if (!lsystem || count <= 0 || !paths) return;
    try {
        loadModel(paths[0]);
    } catch (const std::exception& e) {
        std::cerr << "Could not load dropped file: " << e.what() << std::endl;
        redrawRequested = true;
    }
}

void showContextMenu() {
#ifdef _WIN32
    HMENU root = CreatePopupMenu();
    HMENU models = CreatePopupMenu();
    if (!root || !models) {
        if (root) DestroyMenu(root);
        if (models) DestroyMenu(models);
        std::cerr << "Could not create context menu; keyboard controls remain available.\n";
        return;
    }
    for (std::size_t i = 0; i < modelFilenames.size(); ++i) {
        const std::wstring label = fs::path(modelFilenames[i]).filename().wstring();
        const UINT flags = MF_STRING | (static_cast<int>(i) == lastFilenameIdx ? MF_CHECKED : MF_UNCHECKED);
        AppendMenuW(models, flags, static_cast<UINT_PTR>(MENU_OBJBASE + i), label.c_str());
    }
    AppendMenuW(root, MF_POPUP | (modelFilenames.empty() ? MF_GRAYED : 0),
                reinterpret_cast<UINT_PTR>(models), L"View L-System");
    const bool hasGeometry = lsystem && lsystem->getNumIter() > 0;
    AppendMenuW(root, MF_STRING | ((!hasGeometry || iter == 0) ? MF_GRAYED : 0),
                MENU_PREVITER, L"Previous iteration\tLeft");
    AppendMenuW(root, MF_STRING | (!hasGeometry ? MF_GRAYED : 0),
                MENU_NEXTITER, L"Next iteration\tRight");
    AppendMenuW(root, MF_STRING | (lastFilename.empty() ? MF_GRAYED : 0),
                MENU_REPARSE, L"Reload input\tSpace / R");
    AppendMenuW(root, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(root, MF_STRING, MENU_EXIT, L"Exit\tEsc");
    POINT point{};
    GetCursorPos(&point);
    HWND handle = glfwGetWin32Window(window);
    SetForegroundWindow(handle);
    const UINT alignment = GetSystemMetrics(SM_MENUDROPALIGNMENT) ? TPM_RIGHTALIGN : TPM_LEFTALIGN;
    const int command = static_cast<int>(TrackPopupMenuEx(root,
        TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | alignment,
        point.x, point.y, handle, nullptr));
    DestroyMenu(root); // Also destroys the attached submenu.
    PostMessageW(handle, WM_NULL, 0, 0);
    if (command != 0) menu(command);
#else
    std::cout << "\nInput files (Up/Down to select; dropping a .txt also works):\n";
    for (std::size_t i = 0; i < modelFilenames.size(); ++i)
        std::cout << (static_cast<int>(i) == lastFilenameIdx ? " * " : "   ")
                  << modelFilenames[i] << '\n';
    std::cout << std::flush;
#endif
    redrawRequested = true;
}

void initGLFW() {
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) throw std::runtime_error("GLFW initialization failed; see the error above.");
    glfwReady = true;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    window = glfwCreateWindow(800, 600, "A4 - GLFW", nullptr, nullptr);
    if (!window) throw std::runtime_error("Cannot create an OpenGL 3.3 window; see the GLFW error above.");
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwGetFramebufferSize(window, &width, &height);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetWindowRefreshCallback(window, refreshCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetDropCallback(window, dropCallback);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    std::cout << "GLFW: " << glfwGetVersionString() << '\n'
              << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
}

void cleanup() {
    // GPU buffers and shaders must be destroyed BEFORE destroying their context.
    if (window) glfwMakeContextCurrent(window);
    lsystem.reset();
    if (window) { glfwDestroyWindow(window); window = nullptr; }
    if (glfwReady) { glfwTerminate(); glfwReady = false; }
}

int main(int argc, char** argv) {
    try {
        // Resolve a command-line relative path before locating the asset folder.
        std::string configFile;
        if (argc > 1) configFile = fs::absolute(fs::path(argv[1])).string();
        locateResources(argc > 0 ? argv[0] : nullptr);
        if (configFile.empty()) configFile = "models/tree1.txt";
        findModelFiles();
        initGLFW();
        lsystem = std::make_unique<LSystem>();
        printHelp();
        try {
            loadModel(configFile);
        } catch (const std::exception& e) {
            std::cerr << "Parse error: " << e.what() << std::endl;
        }
        updateTitle();
        // A4 is not animated. Render the initial frame, then wait for input.
        while (!glfwWindowShouldClose(window)) {
            if (popupRequested) { popupRequested = false; showContextMenu(); }
            if (glfwWindowShouldClose(window)) break;
            if (redrawRequested) { redrawRequested = false; display(); }
            glfwWaitEvents();
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        cleanup();
        return 1;
    }
    cleanup();
    return 0;
}
