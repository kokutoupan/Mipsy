# ソースファイルを指定 (分割予定のファイルも含めて記述しておきます)
add_library(codegen
    src/generate.c
    src/gen_addr.c
    src/gen_expr.c
    src/gen_stmt.c
    src/mips_code.c
    src/mips_print.c
    src/mips_reg.c
)

# インクルードパスの設定
target_include_directories(codegen
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include  # 外部(main.c)にも公開
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src      # 内部(internal.h)用
        ${PROJECT_SOURCE_DIR}/include        # ルートのinclude(types.hなど)用
)
