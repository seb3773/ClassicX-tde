#################################################
# Classic-X Applet CMakeLists.txt
#################################################

project( classicxapplet )
cmake_minimum_required( VERSION 3.0 )
include( FindPkgConfig )
include( TDEVersion )
include( TDEMacros )
find_package( TQt )
find_package( TDE )
include( TDESetupPaths )
tde_set_project_version( )

include( TDEMacros )
include( TDESetupPaths )
tde_setup_paths( )

add_definitions(
  -DHAVE_CONFIG_H
  -DWITH_TDEHWLIB
)

option(CLASSICX_AGGRESSIVE_FLAGS "Enable aggressive -O2/LTO/gc-sections compile flags" ON)

if(CLASSICX_AGGRESSIVE_FLAGS)
    set(_CLASSICX_C_FLAGS "-O2 -flto=auto -fstrict-aliasing -ffunction-sections -fdata-sections -fno-ident -fno-plt -fomit-frame-pointer -fno-stack-protector -fno-math-errno -fvisibility=hidden -fvisibility-inlines-hidden -fmerge-all-constants -fno-asynchronous-unwind-tables -DNDEBUG")
    set(_CLASSICX_CXX_EXTRA "-std=c++03 -fno-threadsafe-statics -fno-exceptions -fmerge-all-constants -Wno-deprecated-declarations -fpermissive")
    set(CMAKE_C_FLAGS "${_CLASSICX_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${_CLASSICX_C_FLAGS} ${_CLASSICX_CXX_EXTRA} ${TQT_CXX_FLAGS} -DTQT_THREAD_SUPPORT")
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-O1 -Wl,-Bsymbolic -Wl,--gc-sections -Wl,--as-needed -Wl,--build-id=none -Wl,-z,norelro -flto=auto -s -Wl,--no-undefined")
    set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-O1 -Wl,-Bsymbolic -Wl,--gc-sections -Wl,--as-needed -Wl,--build-id=none -Wl,-z,norelro -flto=auto -s -Wl,--no-undefined")
else()
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TQT_CXX_FLAGS} -DTQT_THREAD_SUPPORT" )
    set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )
    set( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined" )
endif()

include_directories(
  ${TQT_INCLUDE_DIRS}
  ${TDE_INCLUDE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### classicxapplet ##################################

set( target classicxapplet )

set( ${target}_SRCS
  classicx_applet.cpp
  panelbutton.cpp
  kickertip.cpp
  kshadowengine.cpp
  kshadowsettings.cpp
  menumanager.cpp
  k_mnu_stub.cpp
  client_mnu.cpp
  k_mnu.cpp
  service_mnu.cpp
  popupmenutitle.cpp
  recentapps.cpp
  classicx_searchlineedit.cpp
  menuinfo.cpp
  dmctl.cpp
  popupmenutop.cpp
  global.cpp
  quickbrowser_mnu.cpp
  browser_mnu.cpp
  embedded_icons.cpp
  x11opacity.cpp
  powermanager_flags.cpp
  classicx_settings_dialog.cpp
  classicx_profile.cpp
)

set_source_files_properties(
  classicx_settings_dialog.cpp
  classicx_profile.cpp
  dmctl.cpp
  kickertip.cpp
  powermanager_flags.cpp
  x11opacity.cpp
  classicxSettings.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/classicxSettings.cpp
  PROPERTIES COMPILE_FLAGS "-Os"
)

tde_add_kcfg_files( ${target}_SRCS classicxSettings.kcfgc )

tde_add_dcop_skels( ${target}_SRCS k_mnu.h )

# Prefix-aware lookup (avoids hardcoded /opt/trinity/lib/...).
find_library( KICKERMAIN_LIBRARY NAMES kickermain
  HINTS ${TDE_LIB_DIR} ${LIB_INSTALL_DIR}
)
if( NOT KICKERMAIN_LIBRARY )
  message( FATAL_ERROR "libkickermain not found (HINTS: ${TDE_LIB_DIR} ${LIB_INSTALL_DIR})" )
endif()

tde_add_kpart( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK tdehw-shared tdecore-shared tdeui-shared tdeio-shared Xau ${KICKERMAIN_LIBRARY}
  DESTINATION ${PLUGIN_INSTALL_DIR}
)

find_program(SSTRIP_EXECUTABLE sstrip)
if(SSTRIP_EXECUTABLE AND CLASSICX_AGGRESSIVE_FLAGS)
    add_custom_command(TARGET ${target}-module POST_BUILD
        COMMAND ${SSTRIP_EXECUTABLE} $<TARGET_FILE:${target}-module>
        COMMENT "Aggressively stripping ${target}.so with sstrip"
    )
endif()

##### desktop files ###############################

tde_create_translated_desktop(
  SOURCE classicxapplet.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kicker/applets
)

##### icons #######################################

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../CX.png
  DESTINATION ${ICON_INSTALL_DIR}/crystalsvg/64x64/apps
  RENAME classicxapplet.png
)
install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../CX.png
  DESTINATION ${ICON_INSTALL_DIR}/hicolor/64x64/apps
  RENAME classicxapplet.png
)

