# ── 单元测试 ──
add_executable(hello_contact_tests
    test_core.cpp
)
target_link_libraries(hello_contact_tests PRIVATE hello_contact_core)
