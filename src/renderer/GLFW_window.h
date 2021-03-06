#pragma once
#include "window.h"

struct GLFWwindow;

namespace Renderer
{
	/**
	* @brief Small utility to create and handle windows platform independently
	*/
	class GLFW_Window : public Window
	{
	public:
		/**
		* @brief Creates the window using the given parameters. The window will stay open as long as the object exists
		*/
		GLFW_Window(unsigned int topX, unsigned int topY, unsigned int width, unsigned int height, const std::string& title);

		/**
		* @brief Closes the window 
		*/
		~GLFW_Window();

		/**
		* @brief Polls the window for events and calls the given callback function for each event
		*/
		virtual void pollEvent(const std::function<void(Event)>& callback) override;

		/**
		* @brief Returns the OS-Specific handle to this window as a void*
		*/
		virtual void* getNativeHandle() override;

		/**
		* @brief Returns the GLFW-handle of this window
		*/
		GLFWwindow* getGLFWwindow();

		/**
		* @brief Sets the title of the window
		*/
		virtual void setWindowTitle(const std::string& title) override;

		/**
		* @brief Gets the current keystate
		*/
        virtual bool getKeyPressed(Utils::EKey key) override {return m_KeyPresses[key];}

		/**
		 * @brief Semi-Private function to set a key.
		 */
		void setKeyPressed(Utils::EKey key, bool value){m_KeyPresses[key] = value;}

		/**
		 * @brief Switches to fullscreen/windowed
		 */
		void switchMode(bool fullscreen);

		/** 
		* @brief Returns whether the window is in fullscreen-mode
		*/
		virtual bool isFullscreen(){ return m_IsFullscreen;}
	private:

		/**
		* @brief Window-Handle of this object 
		*/
		GLFWwindow* m_pWindowHandle;

		/**
		 * @brief state of all keys
		 */
		bool m_KeyPresses[Utils::KEY_LAST];

        /**
         * @brief indicates if a menu is opened
         */
        bool m_InMenu;

		/**
		 * @brief True, if this is a fullscreen-window
		 */
		bool m_IsFullscreen;

		/**
		 * @brief Start coords for the window
		 */
		int m_TopX, m_TopY, m_Width, m_Height;
	};
}
