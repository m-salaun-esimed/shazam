# Configuration de CMake pour TP C++ Moderne esimed
cmake_minimum_required(VERSION 3.29)

# Nom du projet et de l'exécutable généré
project(template_cpp_cmake)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
# Forcer l'utilisation de C++ 23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Tout doit être parfait
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
# Ne pas utiliser les extensions de CMake
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF)
endif()
include(cmake/compile_options.cmake)

# Répertoire contenant les sources des exercices
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
# Répertoire contenant les sources des tests unitaires
set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

#######################################################
# Partie à mettre à jour manuellement
# Liste des fichiers sources des modules des exercices
# Exécutable principal
add_executable(${PROJECT_NAME}
        ${SRC_DIR}/main.cpp
)
target_sources(${PROJECT_NAME}
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        ${SRC_DIR}/exo1.ixx
)
compile_options(${PROJECT_NAME})

# Exécutable et liste des fichiers sources des tests unitaires
add_executable(tests ${TEST_FILES}
        ${TESTS_DIR}/exo1.cpp
)
target_sources(tests
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        ${SRC_DIR}/exo1.ixx
)
#######################################################

# bibliothèque pour les tests unitaires
include(cmake/catch2.cmake)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain std-cxx-modules)
include(CTest)
include(Catch)
catch_discover_tests(tests)

# Pour "import std"
include(cmake/std.cmake)
