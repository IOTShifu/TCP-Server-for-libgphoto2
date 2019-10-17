#ifndef COMMUNICATOR_H_
#define COMMUNICATOR_H_

#define CAM_NAME_MAX_LEN 50
#define CAM_PORT_MAX_LEN 15

#define NUM_OF_SUPPORTED_CAMS_AT_A_TIME 4


#include <syslog.h>
#include <string.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>

#include <gphoto2pp/Commands.hpp>


#include <gphoto2pp/helper_gphoto2.hpp>
#include <gphoto2pp/camera_list_wrapper.hpp>
#include <gphoto2pp/exceptions.hpp>
#include <gphoto2pp/helper_camera_wrapper.hpp>
#include <gphoto2pp/camera_wrapper.hpp>
#include <gphoto2pp/camera_file_wrapper.hpp>
//Widget types
#include <gphoto2pp/date_widget.hpp>
#include <gphoto2pp/toggle_widget.hpp>
#include <gphoto2pp/range_widget.hpp>
#include <gphoto2pp/menu_widget.hpp>
#include <gphoto2pp/radio_widget.hpp>
#include <gphoto2pp/text_widget.hpp>
#include <gphoto2pp/window_widget.hpp>
#include <gphoto2pp/camera_file_wrapper.hpp>
#include <gphoto2pp/camera_file_type_wrapper.hpp>
#include <gphoto2pp/camera_file_path_wrapper.hpp>
#include <gphoto2pp/camera_event_type_wrapper.hpp>
#include <gphoto2pp/camera_capture_type_wrapper.hpp>
#include <libusb-1.0/libusb.h>


namespace gphoto2
{
#include <gphoto2/gphoto2-camera.h>
#ifndef GPHOTO_LESS_25
#include <gphoto2/gphoto2-abilities-list.h> // Only needed for the pre 2.5 initialize method (because _autodetect doesn't exist)
#endif
#include <gphoto2/gphoto2-file.h>
}
#include <sstream>

#include <chrono>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>   
#include <unistd.h>
#include <fstream>
#include <errno.h>

#include <netinet/in.h>
#include <gphoto2/gphoto2-context.h>


#ifndef TCP_USER_TIMEOUT
    #define TCP_USER_TIMEOUT 18  // how long for loss retry before timeout [ms]
#endif

#define PTP_HEADER 12

#define DDS_MAJOR  1
#define DDS_MINOR  1
#define DDS_NAME "Shifu DSLR Controller"


struct PtpPacket{
	uint32_t packet_len;
	uint16_t packet_type;
	uint16_t packet_command;
	uint32_t session_ID;
};

struct MyTestHandlerClass
{
    void mySuperSpecialHandler(const gphoto2pp::CameraFilePathWrapper& cameraFilePath, const std::string& data) const
    {
        std::cout << "My Super Special Handler was Triggered with file: " << cameraFilePath.Name << std::endl;
    }
};

struct Cam
{
    std::string model;
    std::string port;
};


//From Client to Server packet structure

struct StructCommuniationPacket{
	
	uint16_t packet_command;
    uint16_t camera_number;
	uint16_t widget_type;
	char  widget_id[50];
	char  widget_value[50];
};

struct  ConfirmationMessage
{
    uint16_t packet_command;
    uint16_t camera_number;

};

struct ImageDetails
{
    char image_name[50];
    char image_folder[100];
};

struct ImageDownloadReqPacket{

    uint16_t packet_command;
    uint16_t camera_number;
    char  image_name[50];
    char  image_folder[100];
};



class ShifuCommunicator {


public:
	ShifuCommunicator();
	virtual ~ShifuCommunicator();

	void 		 handleClientConnection(int socket);	
	bool 		 SendLiveStreamToClient(int socket);//int socket,struct sockaddr_in,unsigned int len);
    void         DownloadImageFromFileWrapper(int socket);
	
    gphoto2pp::CameraWrapper 	*mCameraWrapper[NUM_OF_SUPPORTED_CAMS_AT_A_TIME];
    gphoto2pp::CameraFilePathWrapper *mCameraFilePath = NULL;
    gphoto2pp::CameraFileWrapper mCameraFileWrapper;
    void SetCameraFree();


private:
	 int			 mSocket;


        std::vector<char>            mCapturedImageBuffer;
   // std::string                  mCapturedImageSize;

	 bool		 mIsLiveViewUp[NUM_OF_SUPPORTED_CAMS_AT_A_TIME];
	
	//pthread_t    clientThread;
	uint16_t 	 mVendorId;
	uint16_t 	 mProductId;
	
	
	//ShifuCommandsCS shifuCommandsCS;
	
	bool 		isSocketInitialized();
	void 		sendWelcomeMessage();
	bool 		readFromClient();
	bool 		processPacket(uint8_t *buf, int size) ;
	
	int 		sendResponsePacket(uint16_t responseCode, uint32_t sessionId);
	void 		setPtpHeader(uint8_t *buf, int offset, int size, uint16_t packetType, uint16_t packetCommand, uint32_t sessionId);
	int 		sendBuffer(uint8_t *buf, int size);
	
	void 		getWidgetCurrentValue(uint8_t *buf);
	void 		getWidgetPossibleValues(uint8_t *buf);
	void 		setWidgetNewValue(uint8_t *buf);
    int         GetConnectedCamerasList(uint8_t *buf);
    void        DownloadFullSizeImage(uint8_t *buf);
	
	void 		captureImage(int);


	
	void 		initCameraWrapper(int camID);
	void 		StopLivePreview(int);
	
	bool 		iAmInCaptureImage[NUM_OF_SUPPORTED_CAMS_AT_A_TIME];



    Cam cam[NUM_OF_SUPPORTED_CAMS_AT_A_TIME];

};

#endif /* COMMUNICATOR_H_ */
