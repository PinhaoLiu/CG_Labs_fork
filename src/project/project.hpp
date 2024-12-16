#pragma once

#include "core/WindowManager.hpp"
#include "core/FPSCamera.h"


class Window;

namespace edan35
{
	//! \brief Wrapper class for Project
	class Project {
	public:
		//! \brief Default constructor.
		//!
		//! It will initialise various modules of bonobo and retrieve a
		//! window to draw to.
		Project(WindowManager& windowManager);


		//! \brief Default destructor.
		//!
		//! It will release the bonobo modules initialised by the
		//! constructor, as well as the window.
		~Project();

		//! \brief Contains the logic of the project, along with the
		//! render loop.
		void run();

	private:
		FPSCameraf	   mCamera;
		InputHandler   inputHandler;
		WindowManager& mWindowManager;
		GLFWwindow*	   window;
	};
}
