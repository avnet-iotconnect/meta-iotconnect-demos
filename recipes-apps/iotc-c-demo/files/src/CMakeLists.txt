cmake_minimum_required(VERSION 3.0.2)

project(iotc-c-demo)

# don't know how to best do this. The intent is to add our include path and the define in order to pick up iotcl_config.h
add_compile_options(-DIOTCL_USER_CONFIG_FILE=\"iotcl_config.h\" -I${CMAKE_CURRENT_SOURCE_DIR}/)

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../iotc-generic-c-sdk SdkSources)
add_subdirectory(${IOTC_C_SDK_DIR} SdkSources)


# adding this for cleanness but the add_compile_options above already took care of it
# include_directories(config)

add_executable(iotc-c-demo main.c)

    # expect warnings about usleep and unused arguments from paho
target_compile_options(iotc-c-demo PRIVATE -std=gnu17 -Wall -Wextra)
target_compile_options(iotc-c-generic-sdk PRIVATE -std=gnu17 -Wall -Wextra)


target_link_libraries(iotc-c-demo iotc-c-generic-sdk)
