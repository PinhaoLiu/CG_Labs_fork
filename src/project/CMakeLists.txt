# Project
add_executable (EDAN35_Project)
target_sources (
	EDAN35_Project
	PRIVATE
		[[project.hpp]]
		[[project.cpp]]
		[[mTools.hpp]]
		[[mTools.cpp]]
		[[fluidSystem.hpp]]
		[[fluidSystem.cpp]])
target_link_libraries (
	EDAN35_Project
	PRIVATE assignment_setup parametric_shapes
)
copy_dlls (EDAN35_Project "${CMAKE_CURRENT_BINARY_DIR}")

install (
	TARGETS
		EDAN35_Project
	DESTINATION	[[bin]]
)

