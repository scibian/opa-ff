# Microsoft Developer Studio Project File - Name="Cmkernel" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Generic Project" 0x010a

CFG=Cmkernel - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Cmkernel.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Cmkernel.mak" CFG="Cmkernel - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Cmkernel - Win32 Release" (based on "Win32 (x86) Generic Project")
!MESSAGE "Cmkernel - Win32 Debug" (based on "Win32 (x86) Generic Project")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
MTL=midl.exe

!IF  "$(CFG)" == "Cmkernel - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Cmkernel - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Cmkernel___Win32_Debug"
# PROP BASE Intermediate_Dir "Cmkernel___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Cmkernel - Win32 Release"
# Name "Cmkernel - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=..\..\..\Win2K\Ibt\Cm\cm_common.c
# End Source File
# Begin Source File

SOURCE=..\..\..\Win2K\Ibt\Cm\cm_kernel.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=..\..\..\Win2K\Ibt\Cm\cm_ioctl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Win2K\Ibt\Cm\cm_kernel.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\Win2K\Ibt\Cm\makefile
# End Source File
# Begin Source File

SOURCE=..\..\..\Win2K\Ibt\Cm\sources
# End Source File
# End Target
# End Project
