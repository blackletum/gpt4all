include(FetchContent)

set(BUILD_SHARED_LIBS OFF)

# suppress warnings during boost build
add_compile_definitions($<$<COMPILE_LANGUAGE:CXX>:BOOST_ALLOW_DEPRECATED_HEADERS>)

set(GPT4ALL_BOOST_TAG 1.87.0)
FetchContent_Declare(
    boost
    URL "https://github.com/boostorg/boost/releases/download/boost-${GPT4ALL_BOOST_TAG}/boost-${GPT4ALL_BOOST_TAG}-cmake.tar.xz"
    URL_HASH "SHA256=7da75f171837577a52bbf217e17f8ea576c7c246e4594d617bfde7fafd408be5"
)
FetchContent_MakeAvailable(boost)

add_subdirectory(date)
