# ── header-only 核心库 ──
add_library(hello_contact_core INTERFACE)
target_include_directories(hello_contact_core INTERFACE include)
target_compile_features(hello_contact_core INTERFACE cxx_std_17)
