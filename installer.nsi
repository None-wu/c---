; NSIS installer script for GTK Learning Management System
; Unicode version for Chinese support

!include "MUI2.nsh"
!include "LogicLib.nsh"

Unicode true
SetCompressor lzma

Name "学习管理系统"
OutFile "Study_Management_Setup.exe"
InstallDir "$PROGRAMFILES\学习管理系统"
InstallDirRegKey HKLM "Software\学习管理系统" "InstallPath"
RequestExecutionLevel admin

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "SimpChinese"

Section "Main" SEC_MAIN
    SetOutPath "$INSTDIR"
    
    File "study_system.exe"
    File /nonfatal "study_data.txt"
    
    File "libLerc.dll"
    File "libasprintf-0.dll"
    File "libatk-1.0-0.dll"
    File "libatomic-1.dll"
    File "libbrotlicommon.dll"
    File "libbrotlidec.dll"
    File "libbrotlienc.dll"
    File "libbz2-1.dll"
    File "libcairo-2.dll"
    File "libcairo-gobject-2.dll"
    File "libcairo-script-interpreter-2.dll"
    File "libcharset-1.dll"
    File "libcrypto-3-x64.dll"
    File "libdatrie-1.dll"
    File "libdeflate.dll"
    File "libepoxy-0.dll"
    File "libexpat-1.dll"
    File "libffi-8.dll"
    File "libfontconfig-1.dll"
    File "libformw6.dll"
    File "libfreetype-6.dll"
    File "libfribidi-0.dll"
    File "libgailutil-3-0.dll"
    File "libgcc_s_seh-1.dll"
    File "libgdk-3-0.dll"
    File "libgdk_pixbuf-2.0-0.dll"
    File "libgif-7.dll"
    File "libgio-2.0-0.dll"
    File "libgirepository-2.0-0.dll"
    File "libglib-2.0-0.dll"
    File "libgmodule-2.0-0.dll"
    File "libgmp-10.dll"
    File "libgmpxx-4.dll"
    File "libgobject-2.0-0.dll"
    File "libgomp-1.dll"
    File "libgraphite2.dll"
    File "libgthread-2.0-0.dll"
    File "libgtk-3-0.dll"
    File "libharfbuzz-0.dll"
    File "libharfbuzz-gobject-0.dll"
    File "libharfbuzz-subset-0.dll"
    File "libhistory8.dll"
    File "libiconv-2.dll"
    File "libintl-8.dll"
    File "libisl-23.dll"
    File "libjbig-0.dll"
    File "libjpeg-8.dll"
    File "libjson-glib-1.0-0.dll"
    File "liblzma-5.dll"
    File "liblzo2-2.dll"
    File "libmenuw6.dll"
    File "libmpc-3.dll"
    File "libmpdec++-4.dll"
    File "libmpdec-4.dll"
    File "libmpfr-6.dll"
    File "libncurses++w6.dll"
    File "libncursesw6.dll"
    File "libpanelw6.dll"
    File "libpango-1.0-0.dll"
    File "libpangocairo-1.0-0.dll"
    File "libpangoft2-1.0-0.dll"
    File "libpangowin32-1.0-0.dll"
    File "libpcre2-16-0.dll"
    File "libpcre2-32-0.dll"
    File "libpcre2-8-0.dll"
    File "libpcre2-posix-3.dll"
    File "libpixman-1-0.dll"
    File "libpkgconf-7.dll"
    File "libpng16-16.dll"
    File "libpython3.12.dll"
    File "libpython3.dll"
    File "libquadmath-0.dll"
    File "libreadline8.dll"
    File "librsvg-2-2.dll"
    File "libsharpyuv-0.dll"
    File "libsqlite3-0.dll"
    File "libssl-3-x64.dll"
    File "libstdc++-6.dll"
    File "libsystre-0.dll"
    File "libtermcap-0.dll"
    File "libthai-0.dll"
    File "libtiff-6.dll"
    File "libtiffxx-6.dll"
    File "libtre-5.dll"
    File "libturbojpeg.dll"
    File "libwebp-7.dll"
    File "libwebpdecoder-3.dll"
    File "libwebpdemux-2.dll"
    File "libwebpmux-3.dll"
    File "libwinpthread-1.dll"
    File "libxml2-16.dll"
    File "libxxhash.dll"
    File "libzstd.dll"
    File "zlib1.dll"

    SetOutPath "$INSTDIR\share\glib-2.0\schemas"
    File "share\glib-2.0\schemas\gschema.dtd"
    File "share\glib-2.0\schemas\gschemas.compiled"
    File "share\glib-2.0\schemas\org.gtk.Demo.gschema.xml"
    File "share\glib-2.0\schemas\org.gtk.Settings.ColorChooser.gschema.xml"
    File "share\glib-2.0\schemas\org.gtk.Settings.Debug.gschema.xml"
    File "share\glib-2.0\schemas\org.gtk.Settings.EmojiChooser.gschema.xml"
    File "share\glib-2.0\schemas\org.gtk.Settings.FileChooser.gschema.xml"
    File "share\glib-2.0\schemas\org.gtk.exampleapp.gschema.xml"

    CreateDirectory "$SMPROGRAMS\学习管理系统"
    CreateShortcut "$SMPROGRAMS\学习管理系统\学习管理系统.lnk" "$INSTDIR\study_system.exe"
    CreateShortcut "$DESKTOP\学习管理系统.lnk" "$INSTDIR\study_system.exe"

    WriteUninstaller "$INSTDIR\uninstall.exe"
    
    WriteRegStr HKLM "Software\学习管理系统" "InstallPath" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\学习管理系统" "DisplayName" "学习管理系统"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\学习管理系统" "UninstallString" '"$INSTDIR\uninstall.exe"'
SectionEnd

Section "Uninstall"
    Delete "$INSTDIR\study_system.exe"
    Delete "$INSTDIR\study_data.txt"
    Delete "$INSTDIR\*.dll"
    
    Delete "$INSTDIR\share\glib-2.0\schemas\*.*"
    RMDir "$INSTDIR\share\glib-2.0\schemas"
    RMDir "$INSTDIR\share\glib-2.0"
    RMDir "$INSTDIR\share"
    
    Delete "$SMPROGRAMS\学习管理系统\*.lnk"
    RMDir "$SMPROGRAMS\学习管理系统"
    
    Delete "$DESKTOP\学习管理系统.lnk"
    
    DeleteRegKey HKLM "Software\学习管理系统"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\学习管理系统"
    
    RMDir "$INSTDIR"
SectionEnd
