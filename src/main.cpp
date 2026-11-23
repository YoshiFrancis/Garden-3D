#include <iostream>
#include <memory>
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include "glstate.hpp"
// glstate.hpp pulls in the Galogen loader (gl_core_3_3.h), so tell GLFW not to
// include any OpenGL headers of its own.
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
namespace fs = std::filesystem;

std::vector<std::string> meshFilenames;		// Paths to .obj files to load

// The window we render into
GLFWwindow* window = nullptr;

// OpenGL state
std::unique_ptr<GLState> glState;

// Initialization functions
void initGLFW();
void findObjFiles();

// Callback functions
void display();
void framebufferSize(GLFWwindow* window, int width, int height);
void key(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseBtn(GLFWwindow* window, int button, int action, int mods);
void mouseMove(GLFWwindow* window, double x, double y);
void scroll(GLFWwindow* window, double xoffset, double yoffset);
void idle();
void cleanup();

// Program entry point
int main(int argc, char** argv) {
	try {
		// Create the window and OpenGL context
		initGLFW();
		findObjFiles();
		// Initialize OpenGL (buffers, shaders, etc.)
		glState = std::unique_ptr<GLState>(new GLState());
		glState->initializeGL();

		// Tell the GLState the initial size of the drawable area
		int fbWidth, fbHeight;
		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
		glState->resizeGL(fbWidth, fbHeight);

	} catch (const std::exception& e) {
		// Handle any errors
		std::cerr << "Fatal error: " << e.what() << std::endl;
		cleanup();
		return -1;
	}

	std::cout << "Mouse controls (can only do this when using the overhead camera):" << std::endl;
	std::cout << "  Left click + drag to rotate camera" << std::endl;
	std::cout << "  Scroll wheel to zoom in/out" << std::endl;
	std::cout << "Keyboard controls (can only do this when using the ground camera):" << std::endl;
	std::cout << "  A:  Turn left" << std::endl;
	std::cout << "  D:  Turn right" << std::endl;
	std::cout << "  W:  Move forward" << std::endl;
	std::cout << "  X:  Move backward" << std::endl;
	std::cout << "  Z:  Move down" << std::endl;
	std::cout << "  C:  Move up" << std::endl;
	std::cout << "  S:  Switch between the two cameras (a ground camera and an overhead camera)" << std::endl;
	std::cout << "  Esc: Quit" << std::endl;
	std::cout << std::endl;

	// Execute main loop
	while (!glfwWindowShouldClose(window)) {
		idle();				// Per-frame updates
		display();			// Draw the scene
		glfwPollEvents();	// Handle input
	}

	cleanup();
	return 0;
}

// Called by GLFW whenever something goes wrong
void errorCallback(int error, const char* description) {
	std::cerr << "GLFW error " << error << ": " << description << std::endl;
}

// Setup window, OpenGL context, and callbacks
void initGLFW() {
	glfwSetErrorCallback(errorCallback);
	if (!glfwInit())
		throw std::runtime_error("Failed to initialize GLFW");

	// Set window and context settings
	int width = 800; int height = 800;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// Required on macOS to get a 3.3 core context
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_DEPTH_BITS, 24);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

	// Create the window
	window = glfwCreateWindow(width, height, "GLFW Window", nullptr, nullptr);
	if (!window) {
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window (is OpenGL 3.3 available?)");
	}

	// Make its context current before doing anything with OpenGL
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);	// Vsync

	// GLFW callbacks
	glfwSetFramebufferSizeCallback(window, framebufferSize);
	glfwSetKeyCallback(window, key);
	glfwSetMouseButtonCallback(window, mouseBtn);
	glfwSetCursorPosCallback(window, mouseMove);
	glfwSetScrollCallback(window, scroll);
}

void findObjFiles() {
	// Search the models/ directory for any file ending in .obj
	fs::path modelsDir = "models";
	for (auto& di : fs::directory_iterator(modelsDir)) {
		if (di.is_regular_file() && di.path().extension() == ".obj")
			meshFilenames.push_back(di.path().string());
	}
	std::sort(meshFilenames.begin(), meshFilenames.end());
}

// Draws a frame
void display() {
	// Tell the GLState to render the scene
	glState->paintGL();

	// Scene is rendered to the back buffer, so swap the buffers to display it
	glfwSwapBuffers(window);
}

// Called when the drawable area is resized (note: on a high-DPI display this is
// bigger than the window size reported by glfwGetWindowSize)
void framebufferSize(GLFWwindow* window, int width, int height) {
	// Tell OpenGL the new framebuffer size
	glState->resizeGL(width, height);
}

// Called when a key is pressed, released, or auto-repeated
void key(GLFWwindow* window, int key, int scancode, int action, int mods) {
	// Only respond to presses (and auto-repeats, so holding a key keeps moving)
	if (action == GLFW_RELEASE) return;

	switch (key) {
	case GLFW_KEY_ESCAPE:	// Quit
		glfwSetWindowShouldClose(window, GLFW_TRUE);
		break;
	case GLFW_KEY_S:	// switch camera
		glState->switchCam();
		break;
	case GLFW_KEY_A:	// turn left
		glState->getCamera(glState->getCamType()).turnLeft();
		break;
	case GLFW_KEY_D:	// turn right
		glState->getCamera(glState->getCamType()).turnRight();
		break;
	case GLFW_KEY_W:	// move forward
		glState->getCamera(glState->getCamType()).moveForward();
		break;
	case GLFW_KEY_X:	// move backward
		glState->getCamera(glState->getCamType()).moveBackward();
		break;
	case GLFW_KEY_C:	// move up
		glState->getCamera(glState->getCamType()).moveUp();
		break;
	case GLFW_KEY_Z:	// move down
		glState->getCamera(glState->getCamType()).moveDown();
		break;
    case GLFW_KEY_I:
        glState->offsetCamera(0.1f);
        break;
    case GLFW_KEY_O:
        glState->offsetCamera(-0.1f);
        break;
	} 
}

// Called when a mouse button is pressed or released
void mouseBtn(GLFWwindow* window, int button, int action, int mods) {
	if (button != GLFW_MOUSE_BUTTON_LEFT) return;

	double x, y;
	glfwGetCursorPos(window, &x, &y);

	// Press left mouse button
	if (action == GLFW_PRESS)
		glState->beginCameraRotate(glm::vec2(x, y));
	// Release left mouse button
	else if (action == GLFW_RELEASE)
		glState->endCameraRotate();		// Stop camera rotation
}

// Called when the mouse moves
void mouseMove(GLFWwindow* window, double x, double y) {
	if (glState->isCamRotating()) {
		// Rotate the camera if currently rotating
		glState->rotateCamera(glm::vec2(x, y));
	}
}

// Called when the scroll wheel moves
void scroll(GLFWwindow* window, double xoffset, double yoffset) {
	if (yoffset > 0)
		glState->offsetCamera(-0.1f);	// "Zoom in"
	else if (yoffset < 0)
		glState->offsetCamera(0.1f);	// "Zoom out"
}

// Called once per frame, before drawing
void idle() {
	// TODO: anything that happens every frame (e.g. movement) should be done here
	// The scene is redrawn every iteration of the main loop, so there is nothing
	// else you need to do to make an update show up on screen
}

// Called when the window is closed or the event loop is otherwise exited
void cleanup() {
	// Delete the GLState object, calling its destructor, which releases the
	// OpenGL objects. This has to happen while the context is still alive.
	glState.reset(nullptr);

	if (window) {
		glfwDestroyWindow(window);
		window = nullptr;
	}
	glfwTerminate();
}
