# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
#SET(CMAKE_C_COMPILER
#    arm-linux-gnueabihf-gcc
#    )

#SET(CMAKE_CXX_COMPILER
#    arm-linux-gnueabihf-g++
#    )

# where is the target environment
SET(CMAKE_FIND_ROOT_PATH
    /storage/raspbian/chroot-raspbian-jessie-armhf
    )

set(arch arm-linux-gnueabihf)

set(CMAKE_C_COMPILER ${arch}-gcc)
set(CMAKE_C_COMPILER_TARGET ${arch})
set(CMAKE_CXX_COMPILER ${arch}-g++)
set(CMAKE_CXX_COMPILER_TARGET ${arch})


# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

include_directories (BEFORE /storage/raspbian/chroot-raspbian-jessie-armhf/usr/include/arm-linux-gnueabihf/ 
    )
