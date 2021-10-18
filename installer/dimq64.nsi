; NSIS installer script for dimq

!include "MUI2.nsh"
!include "nsDialogs.nsh"
!include "LogicLib.nsh"

; For environment variable code
!include "WinMessages.nsh"
!define env_hklm 'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'

Name "Eclipse dimq"
!define VERSION 1.0.0
OutFile "dimq-${VERSION}-install-windows-x64.exe"

!include "x64.nsh"
InstallDir "$PROGRAMFILES64\dimq"

;--------------------------------
; Installer pages
!insertmacro MUI_PAGE_WELCOME

!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH


;--------------------------------
; Uninstaller pages
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

;--------------------------------
; Languages
!insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Installer sections

Section "Files" SecInstall
	SectionIn RO
	SetOutPath "$INSTDIR"
	File "..\build64\src\Release\dimq.exe"
	File "..\build64\apps\dimq_ctrl\Release\dimq_ctrl.exe"
	File "..\build64\apps\dimq_passwd\Release\dimq_passwd.exe"
	File "..\build64\client\Release\dimq_pub.exe"
	File "..\build64\client\Release\dimq_sub.exe"
	File "..\build64\client\Release\dimq_rr.exe"
	File "..\build64\lib\Release\dimq.dll"
	File "..\build64\lib\cpp\Release\dimqpp.dll"
	File "..\build64\plugins\dynamic-security\Release\dimq_dynamic_security.dll"
	File "..\aclfile.example"
	File "..\ChangeLog.txt"
	File "..\dimq.conf"
	File "..\NOTICE.md"
	File "..\pwfile.example"
	File "..\README.md"
	File "..\README-windows.txt"
	File "..\README-letsencrypt.md"
	;File "C:\pthreads\Pre-built.2\dll\x64\pthreadVC2.dll"
	File "C:\OpenSSL-Win64\bin\libssl-1_1-x64.dll"
	File "C:\OpenSSL-Win64\bin\libcrypto-1_1-x64.dll"
	File "..\edl-v10"
	File "..\epl-v20"

	SetOutPath "$INSTDIR\devel"
	File "..\build64\lib\Release\dimq.lib"
	File "..\build64\lib\cpp\Release\dimqpp.lib"
	File "..\include\dimq.h"
	File "..\include\dimq_broker.h"
	File "..\include\dimq_plugin.h"
	File "..\include\mqtt_protocol.h"
	File "..\lib\cpp\dimqpp.h"

	WriteUninstaller "$INSTDIR\Uninstall.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dimq64" "DisplayName" "Eclipse dimq MQTT broker (64 bit)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dimq64" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dimq64" "QuietUninstallString" "$\"$INSTDIR\Uninstall.exe$\" /S"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dimq64" "HelpLink" "https://dimq.org/"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dimq64" "URLInfoAbout" "https://dimq.org/"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dimq64" "DisplayVersion" "${VERSION}"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dimq64" "NoModify" "1"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dimq64" "NoRepair" "1"

	WriteRegExpandStr ${env_hklm} dimq_DIR $INSTDIR
	SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
SectionEnd

Section "Service" SecService
	ExecWait '"$INSTDIR\dimq.exe" install'
SectionEnd

Section "Uninstall"
	ExecWait '"$INSTDIR\dimq.exe" uninstall'
	Delete "$INSTDIR\dimq.exe"
	Delete "$INSTDIR\dimq_ctrl.exe"
	Delete "$INSTDIR\dimq_passwd.exe"
	Delete "$INSTDIR\dimq_pub.exe"
	Delete "$INSTDIR\dimq_sub.exe"
	Delete "$INSTDIR\dimq_rr.exe"
	Delete "$INSTDIR\dimq.dll"
	Delete "$INSTDIR\dimqpp.dll"
	Delete "$INSTDIR\dimq_dynamic_security.dll"
	Delete "$INSTDIR\aclfile.example"
	Delete "$INSTDIR\ChangeLog.txt"
	Delete "$INSTDIR\dimq.conf"
	Delete "$INSTDIR\pwfile.example"
	Delete "$INSTDIR\README.txt"
	Delete "$INSTDIR\README-windows.txt"
	Delete "$INSTDIR\README-letsencrypt.md"
	;Delete "$INSTDIR\pthreadVC2.dll"
	Delete "$INSTDIR\libssl-1_1-x64.dll"
	Delete "$INSTDIR\libcrypto-1_1-x64.dll"
	Delete "$INSTDIR\edl-v10"
	Delete "$INSTDIR\epl-v20"

	Delete "$INSTDIR\devel\dimq.h"
	Delete "$INSTDIR\devel\dimq.lib"
	Delete "$INSTDIR\devel\dimq_broker.h"
	Delete "$INSTDIR\devel\dimq_plugin.h"
	Delete "$INSTDIR\devel\dimq_plugin.h"
	Delete "$INSTDIR\devel\dimqpp.h"
	Delete "$INSTDIR\devel\dimqpp.lib"
	Delete "$INSTDIR\devel\mqtt_protocol.h"

	Delete "$INSTDIR\Uninstall.exe"
	RMDir "$INSTDIR"
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dimq64"

	DeleteRegValue ${env_hklm} dimq_DIR
	SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
SectionEnd

LangString DESC_SecInstall ${LANG_ENGLISH} "The main installation."
LangString DESC_SecService ${LANG_ENGLISH} "Install dimq as a Windows service?"

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${SecInstall} $(DESC_SecInstall)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecService} $(DESC_SecService)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

