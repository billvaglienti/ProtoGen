cmake_minimum_required(VERSION 3.21)
project(ProtoGenTestCmake)

find_package(ProtoGen CONFIG REQUIRED)

# Now you can use the protegen_generate function.
protegen_generate(
    INPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/../exampleprotocol.xml"
    OUTPUT_PATH "out"
    NO_MARKDOWN
    SHOW_HIDDEN
)
