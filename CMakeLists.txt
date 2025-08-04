cmake_minimum_required(VERSION 3.16)

project(shooting_game VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ✅ 这里要同时找 Widgets + Multimedia
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Multimedia)

set(PROJECT_SOURCES
    main.cpp
    widget.cpp
    widget.h
    widget.ui
)

qt_add_resources(GAME_RESOURCES Resources/game_character.qrc)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Core)

    qt_add_executable(shooting_game
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        player.h player.cpp
        animation.h animation.cpp
        game_character.qrc
        map.h map.cpp
        playercontroller.h playercontroller.cpp
        CollisionResult.h
        combatmanager.h combatmanager.cpp
        dropitem.h dropitem.cpp
        entity.h entity.cpp
        ball.h ball.cpp
        bullet.h bullet.cpp
        soundmanager.h soundmanager.cpp
        gameai.h gameai.cpp
    )
else()
    if(ANDROID)
        add_library(shooting_game SHARED ${PROJECT_SOURCES})
    else()
        add_executable(shooting_game ${PROJECT_SOURCES})
    endif()
endif()

# ✅ 这里必须把 Multimedia 链接进去
target_link_libraries(shooting_game
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Multimedia
)
target_link_libraries(shooting_game PRIVATE Qt6::Core)

# ✅ 下面这些不用动
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.shooting_game)
endif()
set_target_properties(shooting_game PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS shooting_game
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(shooting_game)
endif()
