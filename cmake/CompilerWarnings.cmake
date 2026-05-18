add_library(project_warnings INTERFACE)

target_compile_options(project_warnings INTERFACE
    -Wall -Wextra -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wcast-align
    -Woverloaded-virtual
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wlogical-op
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    $<$<BOOL:${STRICT_WARNINGS}>:-Werror>
)
