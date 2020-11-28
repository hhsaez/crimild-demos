SET( CRIMILD_APP_NAME ImGUI )
SET( CRIMILD_APP_SOURCE_DIRECTORIES "." )
SET( CRIMILD_APP_INCLUDE_DIRECTORIES "." )
SET( CRIMILD_APP_INDEX_FILE "./index.html" )

SET( CRIMILD_APP_INCLUDE_DIRECTORIES
	 ${CRIMILD_APP_INCLUDE_DIRECTORIES}
	 ${CRIMILD_SOURCE_DIR}/core/src
	 ${CRIMILD_SOURCE_DIR}/third-party/imgui
	 ${CRIMILD_SOURCE_DIR}/third-party/imgui/examples
)

SET ( CRIMILD_APP_SOURCE_FILES
	 ${CRIMILD_SOURCE_DIR}/third-party/imgui/imgui.cpp
	 ${CRIMILD_SOURCE_DIR}/third-party/imgui/imgui_demo.cpp
	 ${CRIMILD_SOURCE_DIR}/third-party/imgui/imgui_draw.cpp
	 ${CRIMILD_SOURCE_DIR}/third-party/imgui/imgui_widgets.cpp
	  ${CRIMILD_SOURCE_DIR}/third-party/imgui/examples/imgui_impl_glfw.cpp
	  ${CRIMILD_SOURCE_DIR}/third-party/imgui/examples/imgui_impl_vulkan.cpp
)

INCLUDE( ModuleBuildApp )

