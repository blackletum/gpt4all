set(TARGET gpt4all-backend)

add_library(${TARGET} STATIC
    json_helpers.cpp
    main.cpp
)
target_compile_features(${TARGET} PUBLIC cxx_std_23)
gpt4all_add_warning_options(${TARGET})
target_include_directories(${TARGET} PRIVATE
    .
    ../include/gpt4all-backend
)
target_link_libraries(${TARGET} PUBLIC
    Boost::describe
    QCoro6::Coro
    Qt6::Core
    Qt6::Network
)
target_link_libraries(${TARGET} PRIVATE
    QCoro6::Network
    fmt::fmt
)

# link Boost::json as -isystem to suppress -Wundef
get_target_property(LIB_INCLUDE_DIRS Boost::json INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(${TARGET} SYSTEM PRIVATE ${LIB_INCLUDE_DIRS})
target_link_libraries(${TARGET} PRIVATE Boost::json)
