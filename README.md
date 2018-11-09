# Monocle by Govind Parmar
# Submission for Amazon Aurora online hackathon

Corporate/educational monitoring solution that takes periodic screenshots, a list of all currently open windows, and a list of visited websites (beta!) to an Amazon Aurora MySQL database periodically which can later be viewed with a Log Viewer.

Requires the MySQL Connector for C 6.1 to build: https://dev.mysql.com/downloads/connector/c/

Open "setup.sln" in the root of the repository in an installation of Visual Studio 2017 that has the Windows 10 SDK Version 10.0.17134.0 configured.  In Project Properties -> VC++ Directories, configure the Include Directories and Library Directories to include the MySQL Connector C 6.1 "include" and "lib" directories, respectively.

Compile with the configuration "Release | x86".
