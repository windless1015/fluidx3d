#pragma once

#include "../core/defines.hpp"
#include <functional>

struct GLFWwindow;

struct GlfwCallbacks {
	std::function<void(int key, bool pressed)> on_key;
	std::function<void(double x, double y)> on_mouse_move;
	std::function<void(double xoffset, double yoffset)> on_scroll;
	std::function<void(int button, bool pressed)> on_mouse_button;
};

class GlfwWindow {
public:
	bool initialize_fullscreen(const char* title, unsigned int& width, unsigned int& height, unsigned int& fps_limit);
	bool initialize_windowed(const char* title, unsigned int& width, unsigned int& height, unsigned int& fps_limit);
	void set_callbacks(const GlfwCallbacks& callbacks);
	void poll();
	void swap();
	bool should_close() const;
	GLFWwindow* handle() const { return window; }
	void set_cursor_pos(const double x, const double y);
	void set_cursor_visible(const bool visible);
	void shutdown();

private:
	GLFWwindow* window = nullptr;
	GlfwCallbacks callbacks;
	friend void on_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	friend void on_cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
	friend void on_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	friend void on_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
};
