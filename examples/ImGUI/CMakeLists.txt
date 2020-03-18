SET( CRIMILD_APP_NAME ImGUI )
SET( CRIMILD_APP_SOURCE_DIRECTORIES "." )
SET( CRIMILD_APP_INCLUDE_DIRECTORIES "." )
SET( CRIMILD_APP_INDEX_FILE "./index.html" )

SET ( CRIMILD_APP_SOURCE_FILES
	  ${CRIMILD_SOURCE_DIR}/third-party/imgui/examples/imgui_impl_glfw.cpp
	  ${CRIMILD_SOURCE_DIR}/third-party/imgui/examples/imgui_impl_vulkan.cpp
)

INCLUDE( ModuleBuildApp )

