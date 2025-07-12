cmake_minimum_required(VERSION 3.15)
project(ProtoGen)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define target
add_executable(ProtoGen)
target_sources(ProtoGen
    PUBLIC
        prebuiltSources/floatspecial.h
        protocolfloatspecial.h
        protocolparser.h
        protocolpacket.h
        protocolfield.h
        enumcreator.h
        protocolfile.h
        protocolscaling.h
        fieldcoding.h
        encodable.h
        protocolstructure.h
        protocolstructuremodule.h
        protocolsupport.h
        encodedlength.h
        shuntingyard.h
        protocolcode.h
        protocolbitfield.h
        protocoldocumentation.h
        tinyxml/tinyxml2.h
    PRIVATE
        main.cpp
        prebuiltSources/floatspecial.c
        protocolfloatspecial.cpp
        protocolparser.cpp
        protocolpacket.cpp
        protocolfield.cpp
        enumcreator.cpp
        protocolfile.cpp
        protocolscaling.cpp
        fieldcoding.cpp
        encodable.cpp
        protocolstructure.cpp
        protocolstructuremodule.cpp
        protocolsupport.cpp
        encodedlength.cpp
        shuntingyard.cpp
        protocolcode.cpp
        protocolbitfield.cpp
        protocoldocumentation.cpp
        tinyxml/tinyxml2.cpp
)

target_include_directories(ProtoGen
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tinyxml>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/prebuiltSources>
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(ProtoGen PRIVATE _DEBUG)
endif()
