CMAKE_MINIMUM_REQUIRED( VERSION 3.1.0 FATAL_ERROR )

PROJECT( CRIMILD_EXAMPLES )

SET( CRIMILD_ENABLE_SCRIPTING ON CACHE BOOL "Enable scripting module for Crimild" )
SET( CRIMILD_ENABLE_IMPORT ON CACHE BOOL "Enable import module for Crimild" )
SET( CRIMILD_ENABLE_SFML ON CACHE BOOL "Enable sfml module for Crimild" )
SET( CRIMILD_ENABLE_GLFW ON CACHE BOOL "Enable glfw module for Crimild" )
SET( CRIMILD_SCRIPTING_LOG_VERBOSE ON CACHE BOOL "Enable verbose logging for Crimild" )

IF ( APPLE ) 
	SET( CRIMILD_ENABLE_AL ON CACHE BOOL "Enable al module for Crimild" )
	SET( CRIMILD_BUILD_TOOLS ON CACHE BOOL "Enable tools for Crimild" )
	SET( CRIMILD_ENABLE_TESTS ON CACHE BOOL "Enable testing module for Crimild" )
	#SET( CRIMILD_ENABLE_PHYSICS ON CACHE BOOL "Enable physics module for Crimild" )
ENDIF ()

ADD_SUBDIRECTORY( crimild )

ENABLE_TESTING()

# Register custom modules 
SET ( CMAKE_MODULE_PATH "${CRIMILD_SOURCE_DIR}/CMakeTools;${CMAKE_MODULE_PATH}" )

ADD_SUBDIRECTORY( examples )
