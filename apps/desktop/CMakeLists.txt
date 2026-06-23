# ── Qt6 桌面版 ──
set(CMAKE_AUTOMOC ON)

add_executable(hello_contact_desktop
    main.cpp
    MainWindow.hpp
    MainWindow.cpp
)
target_link_libraries(hello_contact_desktop PRIVATE
    hello_contact_core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)
target_include_directories(hello_contact_desktop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
