
# TCP-Server-for-libgphoto2

  When I started using libgphoto2 the main problem was to access it remotely and develop a remote control for my DSLR camera. It works well with gphoto2 (user interface for the libgphoto2) but the problem to solve was that accessing those commands over the wire remotely. With TCP server implementation, it can done easly from any client implementation like code Java, Android App, IOS App or even from other CPP client code.

Try it out in your project and make it more stable and features reach so the remote photogrpahy will become easy for all the photographers.


## Basics

**Tools and libraries you need:**
1) gphoto2pp :   [https://github.com/maldworth/gphoto2pp](https://github.com/maldworth/gphoto2pp)
2) libgphoto2 :  [https://github.com/gphoto/libgphoto2](https://github.com/gphoto/libgphoto2)

### Quick Introduction 
libgphoto2 is a library that can be used by applications to access various digital cameras.

gphoto2pp is a C++11 wrapper for libgphoto2.

This TCP server can become a part of gphot2pp or can be used independently with gphot2pp libraries dynamic linking (For that, you need to modify this code a bit)

## Setup

1) Select a device or a machine where you want to run libgphoto2 and planning to connect your camera over USB port e.g. Ardiuno, Rpi, Windows or Unix laptop.

2) Build and Install libgphoto2 from https://github.com/gphoto/libgphoto2

3) Get the code to your local machine from (DO NOT BUIDL AND INSTALL YET) https://github.com/maldworth/gphoto2pp

4) Get the code from [https://github.com/IOTShifu/TCP-Server-for-libgphoto2](https://github.com/IOTShifu/TCP-Server-for-libgphoto2) and add this to the gphoto2pp's code you just downloaded in above step

5) Now build "gphoto2pp"

6) Run the binary

At this stage, your TCP server is up and running. You can check your system logs for the messages like "ShifuServer starting...", "Starting TCP socket server on port 5555" etc.

## Let's go deep

If you see this code, there are 2 major code files, 
### ShifuServer.cpp :   
main entry for this server taking care of all 3 servers over different sockets (Port numbers are 5555, 4444, 6666)
* Port 5555 -  Command server (communicator)
This server receives all the commands listed in commands.hpp

* Port 4444 -  Live streaming server 
This server creates a channel for the live streaming over TCP with the client, this server will be started only when Command server receivs START_LIVE_VIEW command from the client.
On STOP_LIVE_VIEW command from the client, this server will stop and exit.

* Port 6666 - Captured Image downloader
This server will be up only on DOWNLOAD_IMAGE command received by the command server from the client. Typically after CAPTURE_IMAGE comand and some deilay as per the camera body and settings etc.

### ShifuCommunicator.cpp :   
This class isolates the TCP communication from the business logic, so the user who is not much conformtable with the TCP and network programming, can focus on business logic of handling DSLR using this class.

Important thing to deal with is ptpPacket struct :
```
struct PtpPacket{
	uint32_t packet_len;
	uint16_t packet_type;
	uint16_t packet_command;
	uint32_t session_ID; 
};
```
All the commands received by the command server will be in this format. Client has to build this struct (Dont worry, languages like Java,C# has option to form a Byte stream)







  
  
