
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


e.g.  JAVA Code to set this packet on the client side:

```
 private static byte[] setCommunicationPacket(short packet_command,
                                                 short widget_type,
                                                 String widget_id,
                                                 String widget_value)
    {

        int size = 2; //Size of Packet Command

        //Calcuate Size requried for the packet to send, Min 2 is required to send command
        if(widget_type != Constants.EMPTY)  size+=2;
        if(widget_id != null ){
            size+=50;
            if(widget_value != null) { //if no widget id provided then there will be no widget value for sure, but only widgegt id can be provided and hence this conditon is inside first one
                size+=50;
            }
        }

        ByteBuffer c = ByteBuffer.allocate(size); //We got our size


        c.putShort(0,(short)packet_command); //Sure that this value will always be there

        if(widget_type != Constants.EMPTY)
            c.putShort(2,(short)widget_type);

        if(widget_id != null ) {
            c.position(4);  //Fixed size 50 ( 4 to 53)
            byte[] byteArrW_id = new byte[50];
            byteArrW_id = widget_id.getBytes();
            c.put(byteArrW_id);

            if(widget_value != null) {
                c.position(54);  //Fixed size 50  (54 to 104)
                byte[] byteArrW_val = new byte[50];
                byteArrW_val = widget_value.getBytes();
                c.put(byteArrW_val);
            }

        }


        byte[] returnVal = c.array();

        return returnVal;

    }
```

### One more example of how client code in JAVA can be written to fire a basic command i.e. Get Connected Camera List
 
```
public static final short GET_CAMERAS_LIST   = 0x0902;
public String GetCameraList()
    {

        String strCamName="No Camera Detected!"; //In case of failure

        byte[] cameraListRequestBytes = setCommunicationPacket(Constants.GET_CAMERAS_LIST,
                                                               Constants.EMPTY,
                                                       null,
                                                    null);


        try {
            writeToServerSocket(cameraListRequestBytes);
        }catch(Exception e){
            return strCamName;
        }

        ///Read response from the server on above command
        byte[] response;
        try {
            response = readBytes(200); //Cam name max len is 50 X we would get max 4 cameras connected: hence 200
        }catch(Exception e){
            return strCamName;
        }

        strCamName = new String(response);

        return strCamName.trim();

    }
```

### Or the most exciting one, i.e. Capture Image
```
public static final short CAPTURE_IMAGE    = 0x0002;
public void CaptureImage() {


        byte[] captureRequestBytes = setCommunicationPacket(Constants.CAPTURE_IMAGE,
                Constants.EMPTY,
                null,
                null);

        try {
            writeToServerSocket(captureRequestBytes);
        } catch (Exception e) {
        }

    }
```
