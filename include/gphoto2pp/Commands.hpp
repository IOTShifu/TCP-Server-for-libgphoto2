//Shifu Client Server Communication commands
#define MSG_TYPE_COMMUNICATEION			0x0001

#define CHECK_CONNECTION_TO_CAMERA 		0x0900 
#define GET_CAMERAS_LIST		 		0x0902
#define INIT_CAMERA_MODEL               0X0903


#define CLOSE_CONNECTION		 		0x0999


//Shifu Camera Commands
#define CAPTURE_IMAGE	 		 		0x0002
 
#define GET_THUMBNAIL	 		 		0x0003
#define DOWNLOAD_IMAGE	 		 		0x0004

#define START_LIVE_VIEW	 		 		0x0005
#define STOP_LIVE_VIEW	 		 		0x0006

#define GET_WIDGET_CURRENT_VAL			0x0007
#define GET_WIDGET_POSSIBLE_VALS		0x0008

#define SET_WIDGET_NEW_VAL				0x0009



//Server To Client Messages
#define CONNECTION_OK			 		0x0800


//Widgets 0x0100 to 0x0108
#define DATE_WIDGET 					0x0101
#define TOGGLE_WIDGET 					0x0102
#define BUTTON_WIDGET 					0x0103
#define RANGE_WIDGET 					0x0104
#define MENU_WIDGET 					0x0105
#define RADIO_WIDGET 					0x0106
#define TEXT_WIDGET 					0x0107
#define SECTION_WIDGET 					0x0108

