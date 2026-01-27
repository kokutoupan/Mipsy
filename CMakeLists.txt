cmake_minimum_required(VERSION 3.10)
project(basic_ls C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# flex / bison
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# BISON: src/basic_ls.y → basic_ls.tab.c/h
BISON_TARGET(parser src/basic_ls.y ${CMAKE_CURRENT_BINARY_DIR}/basic_ls.tab.c
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/basic_ls.tab.h)

# FLEX: src/basic_ls.l → lex.yy.c
FLEX_TARGET(scanner src/basic_ls.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)

# Link flex + bison outputs
ADD_FLEX_BISON_DEPENDENCY(scanner parser)

# codegen モジュール
add_subdirectory(codegen)

# main build
add_executable(mipsy
    src/main.c      # パス変更
    src/make_ast.c  # パス変更
    src/print_ast.c
    ${BISON_parser_OUTPUTS}
    ${FLEX_scanner_OUTPUTS}
)

target_include_directories(mipsy PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR} # bison生成ヘッダ用
    include
    ${CMAKE_CURRENT_SOURCE_DIR} # codegenディレクトリへのパス解決用(必要なら)
)

target_link_libraries(mipsy PRIVATE
    codegen
    fl   # libfl
    y    # liby
)
