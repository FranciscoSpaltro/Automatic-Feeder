# src/CMakeLists.txt

# 1. Buscamos los archivos fuente de SRC (tu main)
FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

# 2. Buscamos los archivos fuente de la LIB (el driver)
#    Esto es lo que te faltaba: decirle que compile también wl134.cpp
FILE(GLOB_RECURSE lib_sources ${CMAKE_SOURCE_DIR}/lib/wl134/*.*)

idf_component_register(
    # Registramos AMBOS grupos de archivos para compilar
    SRCS ${app_sources} ${lib_sources}
    
    # Decimos dónde están los .h
    # "."          -> Busca en src/
    # "../lib/wl134" -> Sal de src, entra a lib/wl134 y busca ahí
    INCLUDE_DIRS "." "../lib/wl134"
    
    # Fundamental para que funcione Serial, WiFi, etc.
    REQUIRES arduino
)