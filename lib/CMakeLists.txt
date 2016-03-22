##-----------------------------------------------------------------------------
##  Welcome to Streamripper
##  This CMakeLists.txt file is given to the public domain.
##  
##  The rest of streamripper is available under the GNU GENERAL PUBLIC 
##  LICENSE, v2 or later.  See COPYING for details.
##-----------------------------------------------------------------------------
PROJECT (streamripper_lib)

SET (STREAMRIPPER_LIB_SRC
    argv.c
    charset.c
    errors.c
    filelib.c
    findsep.c
    iconvert.c
    lib.c
    parse.c
    relaylib.c
    rip_manager.c
    ripstream.c
    threadlib.c
    cbuf2.c
    debug.c
    external.c
    findsep2.c
    http.c
    ictest.c
    mchar.c
    prefs.c
    ripaac.c
    ripogg.c
    socklib.c
    utf8.c
    )

IF (NOT IBERTY_FOUND)
  SET (STREAMRIPPER_LIB_SRC
    ${STREAMRIPPER_LIB_SRC}
    argv.c)
ENDIF (NOT IBERTY_FOUND)

ADD_LIBRARY (streamripper1 ${STREAMRIPPER_LIB_SRC})
INSTALL (TARGETS streamripper1 DESTINATION lib)
