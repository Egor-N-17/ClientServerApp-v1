# ClientServerApp-v1
ClientServerApp base on TCP/IP connection. Application read xml file and take command and response message. 
Client send command message to server each second, and server send corresponding responce message.
This version use Win and Linux API for connecting between client and server. To compile app on Linux use ToCompile.txt file
Unit tests can be diabled by deleting UNIT_TEST define from Lib.h file

The structure of project in Windows is next:

\ClientServerApp - folder contain code and another files required for Visual Studio
\boost_1_83_0 - folder contain boost
