# ── 终端版（零依赖） ──
add_executable(hello_contact_terminal main.cpp)
target_link_libraries(hello_contact_terminal PRIVATE hello_contact_core)
