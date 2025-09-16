cmake_minimum_required(VERSION 3.10)
project(fossodoro C)

set(CMAKE_C_STANDARD 99)

# Source files
set(SOURCES
    fossodoro.c
    osd.c
    sound.c
)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(NOTIFY REQUIRED libnotify)
pkg_check_modules(MPG123 REQUIRED libmpg123)
pkg_check_modules(AO REQUIRED ao)
pkg_check_modules(AOSD REQUIRED libaosd)
pkg_check_modules(GLIB REQUIRED glib-2.0)

# X11 and related libraries
find_package(X11 REQUIRED)

# Include directories
include_directories(
    ${GTK3_INCLUDE_DIRS}
    ${NOTIFY_INCLUDE_DIRS}
    ${MPG123_INCLUDE_DIRS}
    ${AO_INCLUDE_DIRS}
    ${AOSD_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${GI18N_INCLUDE_DIRS}
    ${X11_INCLUDE_DIR}
)

# Compiler definitions and options
add_definitions(
    ${GTK3_CFLAGS_OTHER}
    ${NOTIFY_CFLAGS_OTHER}
    ${MPG123_CFLAGS_OTHER}
    ${AO_CFLAGS_OTHER}
    ${AOSD_CFLAGS_OTHER}
)

# Executable
add_executable(fossodoro ${SOURCES})

# Link libraries
target_link_libraries(fossodoro
    ${GTK3_LIBRARIES}
    ${NOTIFY_LIBRARIES}
    ${MPG123_LIBRARIES}
    ${AO_LIBRARIES}
    ${AOSD_LIBRARIES}
    ${X11_LIBRARIES}
    pthread
    Xcomposite
    Xrandr
    cairo
    m
)

# Build translation files (*.po -> *.mo)
file(GLOB PO_FILES "${CMAKE_SOURCE_DIR}/translations/*.po")
foreach(PO_FILE ${PO_FILES})
    get_filename_component(LANG_NAME ${PO_FILE} NAME_WE)
    set(MO_DIR "${CMAKE_SOURCE_DIR}/locale/${LANG_NAME}/LC_MESSAGES")
    set(MO_FILE "${MO_DIR}/fossodoro.mo")
    add_custom_command(
        OUTPUT ${MO_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${MO_DIR}
        COMMAND msgfmt -o ${MO_FILE} ${PO_FILE}
        DEPENDS ${PO_FILE}
        COMMENT "Compiling translation for ${LANG_NAME}"
        VERBATIM
    )
    list(APPEND MO_FILES ${MO_FILE})
endforeach()

if(MO_FILES)
    add_custom_target(translations ALL DEPENDS ${MO_FILES})
    add_dependencies(fossodoro translations)
endif()

# For pkg-config .pc files
link_directories(
    ${GTK3_LIBRARY_DIRS}
    ${NOTIFY_LIBRARY_DIRS}
    ${MPG123_LIBRARY_DIRS}
    ${AO_LIBRARY_DIRS}
    ${AOSD_LIBRARY_DIRS}
)

# Install target (optional)
install(TARGETS fossodoro DESTINATION bin)

add_custom_command(
    TARGET fossodoro POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/icons $<TARGET_FILE_DIR:fossodoro>/icons
)
add_custom_command(
    TARGET fossodoro POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/images $<TARGET_FILE_DIR:fossodoro>/images
)
add_custom_command(
    TARGET fossodoro POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/locale $<TARGET_FILE_DIR:fossodoro>/locale
)