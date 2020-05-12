project(xrResources)

install(FILES fsgame.ltx DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/openxray) 

install(DIRECTORY bash-completion DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/)

install(DIRECTORY gamedata DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/openxray)

install(FILES openxray_cop.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)

install(FILES icons/16x16/openxray_cop.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps/)
install(FILES icons/32x32/openxray_cop.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps/)
install(FILES icons/48x48/openxray_cop.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps/)
install(FILES icons/64x64/openxray_cop.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps/)
install(FILES icons/64x64/openxray_cop.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps/)

install(FILES openxray_cs.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES icons/16x16/openxray_cs.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps/)
install(FILES icons/32x32/openxray_cs.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps/)
install(FILES icons/48x48/openxray_cs.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps/)
install(FILES icons/64x64/openxray_cs.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps/)
install(FILES icons/64x64/openxray_cs.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps/)

install(FILES openxray_soc.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES icons/16x16/openxray_soc.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps/)
install(FILES icons/32x32/openxray_soc.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps/)
install(FILES icons/48x48/openxray_soc.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps/)
install(FILES icons/64x64/openxray_soc.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps/)
install(FILES icons/64x64/openxray_soc.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps/)