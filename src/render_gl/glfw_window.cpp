#include "glfw_window.hpp"
#include "../core/utilities.hpp"
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

static void on_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	GlfwWindow* self = (GlfwWindow*)glfwGetWindowUserPointer(window);
	if(self==nullptr) return;
	if(!self->handle()) return;
	if(!self->callbacks.on_key) return;
	if(action==GLFW_PRESS || action==GLFW_REPEAT) {
		self->callbacks.on_key(key, true);
	} else if(action==GLFW_RELEASE) {
		self->callbacks.on_key(key, false);
	}
}

static void on_cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
	GlfwWindow* self = (GlfwWindow*)glfwGetWindowUserPointer(window);
	if(self==nullptr) return;
	if(self->callbacks.on_mouse_move) self->callbacks.on_mouse_move(xpos, ypos);
}

static void on_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	GlfwWindow* self = (GlfwWindow*)glfwGetWindowUserPointer(window);
	if(self==nullptr) return;
	if(self->callbacks.on_scroll) self->callbacks.on_scroll(xoffset, yoffset);
}

static void on_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	GlfwWindow* self = (GlfwWindow*)glfwGetWindowUserPointer(window);
	if(self==nullptr) return;
	if(!self->callbacks.on_mouse_button) return;
	if(action==GLFW_PRESS) self->callbacks.on_mouse_button(button, true);
	if(action==GLFW_RELEASE) self->callbacks.on_mouse_button(button, false);
}

bool GlfwWindow::initialize_fullscreen(const char* title, unsigned int& width, unsigned int& height, unsigned int& fps_limit) {
	if(!glfwInit()) {
		print_error("Failed to initialize GLFW.");
		return false;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	if(mode==nullptr) {
		print_error("Failed to query GLFW video mode.");
		return false;
	}
	width = (unsigned int)mode->width;
	height = (unsigned int)mode->height;
	fps_limit = (unsigned int)mode->refreshRate;

	window = glfwCreateWindow((int)width, (int)height, title, monitor, nullptr);
	if(window==nullptr) {
		print_error("Failed to create GLFW window.");
		return false;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		print_error("Failed to initialize GLAD.");
		return false;
	}

	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, on_key_callback);
	glfwSetCursorPosCallback(window, on_cursor_pos_callback);
	glfwSetScrollCallback(window, on_scroll_callback);
	glfwSetMouseButtonCallback(window, on_mouse_button_callback);
	return true;
}

void GlfwWindow::set_callbacks(const GlfwCallbacks& callbacks) {
	this->callbacks = callbacks;
}

void GlfwWindow::poll() {
	glfwPollEvents();
}

void GlfwWindow::swap() {
	glfwSwapBuffers(window);
}

bool GlfwWindow::should_close() const {
	return window==nullptr || glfwWindowShouldClose(window);
}

void GlfwWindow::set_cursor_pos(const double x, const double y) {
	if(window!=nullptr) glfwSetCursorPos(window, x, y);
}

void GlfwWindow::set_cursor_visible(const bool visible) {
	if(window==nullptr) return;
	glfwSetInputMode(window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	if(glfwRawMouseMotionSupported()) {
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, visible ? GLFW_FALSE : GLFW_TRUE);
	}
}

void GlfwWindow::shutdown() {
	if(window!=nullptr) {
		glfwDestroyWindow(window);
		window = nullptr;
	}
	glfwTerminate();
}
