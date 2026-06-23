# ── LLM 模块（占位） ──
# TODO: 接入 llama.cpp，实现本地 AI 对话
# 依赖: llama.cpp (https://github.com/ggerganov/llama.cpp)
# add_library(hello_contact_llm STATIC
#     src/llm_agent.cpp
# )
# target_include_directories(hello_contact_llm PUBLIC include)
# target_link_libraries(hello_contact_llm PUBLIC hello_contact_core)
# target_link_libraries(hello_contact_llm PRIVATE llama)
message(WARNING "LLM module not yet implemented — set HELLO_CONTACT_BUILD_LLM=OFF to silence")
