; Script generated by the HM NIS Edit Script Wizard.

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "FreeOrion"
!define PRODUCT_VERSION "0.3.12"
!define PRODUCT_PUBLISHER "FreeOrion Community"
!define PRODUCT_WEB_SITE "http://www.freeorion.org"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\freeorionca.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

SetCompressor bzip2

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "..\default\COPYING"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\freeorion.exe"
!define MUI_FINISHPAGE_RUN_PARAMETERS "--fullscreen 1"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "..\..\FreeOrion-0.3.12-Setup.exe"
InstallDir "$PROGRAMFILES\FreeOrion"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite try
  File "..\..\vcredist_x86.exe"
  ExecWait "$INSTDIR\vcredist_x86.exe /q"
  File /r /x .svn "..\default"
  File "..\GiGi.dll"
  File "..\GiGiOgre.dll"
  File "..\GiGiOgrePlugin_OIS.dll"
  File "..\OIS.dll"
  File "..\OgreMain.dll"
  File "..\OpenAL32.dll"
  File "..\Plugin_OctreeSceneManager.dll"
  File "..\Plugin_ParticleFX.dll"
  File "..\RenderSystem_GL.dll"
  File "..\alut.dll"
  File "..\boost_date_time-vc90-mt-1_36.dll"
  File "..\boost_filesystem-vc90-mt-1_36.dll"
  File "..\boost_iostreams-vc90-mt-1_36.dll"
  File "..\boost_python-vc80-mt-1_36.dll"
  File "..\boost_python-vc90-mt-1_36.dll"
  File "..\boost_regex-vc90-mt-1_36.dll"
  File "..\boost_serialization-vc90-mt-1_36.dll"
  File "..\boost_signals-vc90-mt-1_36.dll"
  File "..\boost_system-vc90-mt-1_36.dll"
  File "..\boost_thread-vc90-mt-1_36.dll"
  File "..\glew32.dll"
  File "..\jpeg.dll"
  File "..\libexpat.dll"
  File "..\libogg.dll"
  File "..\libpng13.dll"
  File "..\libvorbis.dll"
  File "..\libvorbisfile.dll"
  File "..\python25.dll"
  File "..\wrap_oal.dll"
  File "..\z.dll"
  File "..\zlib1.dll"
  File "..\freeorionca.exe"
  File "..\freeoriond.exe"
  File "..\freeorion.exe"
  File "..\freeorionca.exe.manifest"
  File "..\freeoriond.exe.manifest"
  File "..\freeorion.exe.manifest"
  File "..\OISInput.cfg"
  File "..\ogre_plugins.cfg"
  CreateDirectory "$SMPROGRAMS\FreeOrion"
  CreateShortCut "$SMPROGRAMS\FreeOrion\FreeOrion.lnk" "$INSTDIR\freeorion.exe" "--fullscreen"
  CreateShortCut "$SMPROGRAMS\FreeOrion\FreeOrion windowed.lnk" "$INSTDIR\freeorion.exe"
  CreateShortCut "$DESKTOP\FreeOrion.lnk" "$INSTDIR\freeorion.exe" "--fullscreen 1"
SectionEnd

Section -AdditionalIcons
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\FreeOrion\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\FreeOrion\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\freeorionca.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\freeorionca.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd


Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  Delete "$SMPROGRAMS\FreeOrion\Uninstall.lnk"
  Delete "$SMPROGRAMS\FreeOrion\Website.lnk"
  Delete "$DESKTOP\FreeOrion.lnk"
  Delete "$SMPROGRAMS\FreeOrion\FreeOrion.lnk"
  Delete "$SMPROGRAMS\FreeOrion\FreeOrion windowed.lnk"

  RMDir "$SMPROGRAMS\FreeOrion"
  RMDir /r "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd