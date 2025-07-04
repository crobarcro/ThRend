cmake_minimum_required(VERSION 3.10)
project(ThRend CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source and header files
set(SRC ThRend/main.cpp)
set(HEADERS
    ThRend/colormap.h
    ThRend/EmbreeUtils.h
    ThRend/FreeImage.h
    ThRend/materialLoader.h
    ThRend/ONB.h
    ThRend/reflections.h
    ThRend/settingsLoader.h
    ThRend/UCDimporter.h
)

# Include directories (GLM and project always needed)
include_directories(
    ThRend
    ThRend/glm
)

# GLM as header-only INTERFACE target
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ThRend/glm)

# This will now use the path set above to find the correct package
find_package(embree 3 REQUIRED)

# Ensure TBB is also linked if needed by your custom build
find_package(TBB)
if(TBB_FOUND)
    message(STATUS "Found TBB, will link against it.")
    include_directories(${TBB_INCLUDE_DIRS})
    set(EMBREE_LIBRARIES ${TBB_LIBRARIES})
else()
    message(WARNING "TBB not found. Your custom Embree build might fail to link.")
    message(WARNING "You may need to install TBB or point to its location.")
endif()

# --- FreeImage: Prefer pkg-config, then system, then manual system path, then local ---
if(UNIX)
    find_package(PkgConfig)
    pkg_check_modules(FREEIMAGE_PKG freeimage)
endif()

if(FREEIMAGE_PKG_FOUND)
    message(STATUS "Using system FreeImage via pkg-config")
    set(FREEIMAGE_INCLUDE_DIRS ${FREEIMAGE_PKG_INCLUDE_DIRS})
    set(FREEIMAGE_LIBRARIES ${FREEIMAGE_PKG_LIBRARIES})
    include_directories(${FREEIMAGE_INCLUDE_DIRS})
else()
    find_package(FreeImage QUIET)
    if(FreeImage_FOUND)
        message(STATUS "Using system FreeImage: ${FreeImage_INCLUDE_DIRS}")
        set(FREEIMAGE_INCLUDE_DIRS ${FreeImage_INCLUDE_DIRS})
        set(FREEIMAGE_LIBRARIES ${FreeImage_LIBRARIES})
        include_directories(${FREEIMAGE_INCLUDE_DIRS})
    else()
        # Manual fallback for standard system path on Linux
        if(UNIX)
            set(FREEIMAGE_INCLUDE_DIRS /usr/include)
            set(FREEIMAGE_LIBRARIES /usr/lib/x86_64-linux-gnu/libfreeimage.so)
            include_directories(${FREEIMAGE_INCLUDE_DIRS})
            if(EXISTS "${FREEIMAGE_LIBRARIES}")
                message(STATUS "Using FreeImage from standard system path: ${FREEIMAGE_LIBRARIES}")
            else()
                message(STATUS "System FreeImage not found in standard path, using local FreeImage")
                set(FREEIMAGE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ThRend/lib)
                set(FREEIMAGE_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/ThRend/lib/libFreeImage.so)
                include_directories(${FREEIMAGE_INCLUDE_DIRS})
            endif()
        else()
            message(STATUS "System FreeImage not found, using local FreeImage")
            set(FREEIMAGE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ThRend/lib)
            set(FREEIMAGE_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/ThRend/lib/libFreeImage.so)
            include_directories(${FREEIMAGE_INCLUDE_DIRS})
        endif()
    endif()
endif()

# Platform-specific settings
if(WIN32)
    set(SYS_LIBS
        kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32 ole32 oleaut32 uuid odbc32 odbccp32
    )
    set(THREND_LIBS
        ${FREEIMAGE_LIBRARIES}
        ${SYS_LIBS}
    )
else()
    # Linux: link pthread, dl, m, OpenMP if available
    find_package(OpenMP)
    set(THREND_LIBS
        ${FREEIMAGE_LIBRARIES}
        pthread dl m
    )
    if(OpenMP_CXX_FOUND)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    endif()
endif()

# Main executable
add_executable(ThRend ${SRC} ${HEADERS})
target_link_libraries(ThRend PRIVATE embree ${THREND_LIBS} glm)

# Exclude GLM dummy.cpp from all builds
set_source_files_properties(ThRend/glm/detail/dummy.cpp PROPERTIES HEADER_FILE_ONLY TRUE)

# IDE source groups (optional, for clarity)
source_group("Header Files" FILES ${HEADERS})

