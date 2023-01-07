project(xrGamedata)

install(FILES fsgame.ltx DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/openxray) 

install(DIRECTORY gamedata DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/openxray)
