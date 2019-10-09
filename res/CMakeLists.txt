project(xrResources)

install(FILES fsgame.ltx DESTINATION /usr/share/openxray) 

install(DIRECTORY bash-completion DESTINATION /usr/share)

install(DIRECTORY gamedata DESTINATION /usr/share/openxray)

install(FILES openxray_cop.desktop DESTINATION /usr/share/applications)

install(FILES icons/16x16/openxray_cop.png DESTINATION /usr/share/icons/hicolor/16x16/apps/)
install(FILES icons/32x32/openxray_cop.png DESTINATION /usr/share/icons/hicolor/32x32/apps/)
install(FILES icons/48x48/openxray_cop.png DESTINATION /usr/share/icons/hicolor/48x48/apps/)
install(FILES icons/64x64/openxray_cop.png DESTINATION /usr/share/icons/hicolor/64x64/apps/)
install(FILES icons/64x64/openxray_cop.png DESTINATION /usr/share/pixmaps/)

install(FILES openxray_cs.desktop DESTINATION /usr/share/applications)
install(FILES icons/16x16/openxray_cs.png DESTINATION /usr/share/icons/hicolor/16x16/apps/)
install(FILES icons/32x32/openxray_cs.png DESTINATION /usr/share/icons/hicolor/32x32/apps/)
install(FILES icons/48x48/openxray_cs.png DESTINATION /usr/share/icons/hicolor/48x48/apps/)
install(FILES icons/64x64/openxray_cs.png DESTINATION /usr/share/icons/hicolor/64x64/apps/)
install(FILES icons/64x64/openxray_cs.png DESTINATION /usr/share/pixmaps/)

install(FILES openxray_soc.desktop DESTINATION /usr/share/applications)
install(FILES icons/16x16/openxray_soc.png DESTINATION /usr/share/icons/hicolor/16x16/apps/)
install(FILES icons/32x32/openxray_soc.png DESTINATION /usr/share/icons/hicolor/32x32/apps/)
install(FILES icons/48x48/openxray_soc.png DESTINATION /usr/share/icons/hicolor/48x48/apps/)
install(FILES icons/64x64/openxray_soc.png DESTINATION /usr/share/icons/hicolor/64x64/apps/)
install(FILES icons/64x64/openxray_soc.png DESTINATION /usr/share/pixmaps/)


