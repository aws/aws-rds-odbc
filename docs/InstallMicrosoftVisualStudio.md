# Install Microsoft Visual Studio

1. Download [`.vsconfig`](../.vsconfig). This will be used to select the required components during the Visual Studio installation.
2. Download and install Microsoft Visual Studio 2022 community edition from https://visualstudio.microsoft.com/downloads/.
3. After the installer starts, close this inner window.
   ![Close inner window](images/VisualStudioInstall_CloseInnerWindow.png?raw=true "Close inner window")
4. Click on the Available tab.
5. Click on More to the right of Visual Studio Community 2022 and select Import configuration.
Browse to and select where `.vsconfig` was downloaded. Then, click on Review details. The following components will be installed.
   ![.NET Multi-platform App UI development](images/VisualStudioInstall_DotNetDevelopment.png?raw=true ".NET Multi-platform App UI development")
   ![Desktop development with C++](images/VisualStudioInstall_DesktopDevelopmentWithCPP.png?raw=true "Desktop development with C++")
6. After reviewing the installation details, click on Install.
7. When the installation is complete, add the path to `msbuild.exe` to the `Path` user environment variable by doing the following inside PowerShell.
   1. Change to the Visual Studio installation directory. This is usually `C:\Program Files\Microsoft Visual Studio`.
   2. Run `gci -r -fi msbuild.exe` and note the directory where `msbuild.exe` exists. This will usually be `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin`.
   3. Add the directory for `msbuild.exe` to the `Path` user environment variable.
