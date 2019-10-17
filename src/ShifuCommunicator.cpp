#include <gphoto2pp/ShifuCommunicator.hpp>
#include <arpa/inet.h>
#include <gphoto2/gphoto2-file.h>
#include <gphoto2/gphoto2-filesys.h>

ShifuCommunicator::ShifuCommunicator() : mSocket(0), mVendorId(0), mProductId(0)
{
    mCameraWrapper[0] = NULL;
    mCameraWrapper[1] = NULL;
    mCameraWrapper[2] = NULL;
    mCameraWrapper[3] = NULL;

}

ShifuCommunicator::~ShifuCommunicator() {

    //if (isSocketInitialized())
    //  close(mSocket);


}



bool ShifuCommunicator::isSocketInitialized()
{
    return mSocket > 0;
}
void ShifuCommunicator::setPtpHeader(uint8_t *buf, int offset, int size, uint16_t packetType, uint16_t packetCommand, uint32_t sessionId)
{
    PtpPacket *ptpPacket = (PtpPacket *)&buf[offset];
    ptpPacket->packet_len = htole32(size);
    ptpPacket->packet_type = htole16(packetType);
    ptpPacket->packet_command = htole16(packetCommand);
    ptpPacket->session_ID = htole32(sessionId);

}
int ShifuCommunicator::sendResponsePacket(uint16_t responseCode, uint32_t sessionId)
{
    int len = sizeof(PtpPacket) + 4;
    uint8_t *buf = (uint8_t *)malloc(len);
    bzero(&buf[0], len);

    *(uint32_t *)&buf[0] = (uint32_t)htole32(len);

    setPtpHeader(buf, 4, len-4, MSG_TYPE_COMMUNICATEION, responseCode, sessionId);

    int r = sendBuffer(buf, len);

    free(buf);

    return r;
}

int ShifuCommunicator::sendBuffer(uint8_t *buf, int size)
{
    //std::cout << "SEND : " << buf << std::endl;
    int r = write(mSocket, buf, size);
    if (r != size)
        syslog(LOG_ERR, "Error sending packet to client");
    return r;
}



void ShifuCommunicator::sendWelcomeMessage()
{
    int len = 4 + 4 + sizeof(DDS_NAME) + 1;
    uint8_t *buf = (uint8_t *)malloc(len);
    bzero(&buf[0], len);

    *(uint32_t *)&buf[0] = htole32(len);
    *(uint16_t *)&buf[4] = htole16(DDS_MAJOR);
    *(uint16_t *)&buf[6] = htole16(DDS_MINOR);
    memcpy(&buf[8], DDS_NAME, sizeof(DDS_NAME));
    sendBuffer(buf, len);
    free(buf);


}

void ShifuCommunicator::initCameraWrapper(int camID)
{

    if(mCameraWrapper[camID]==NULL) {
        try{
            mCameraWrapper[camID] = new gphoto2pp::CameraWrapper(cam[camID].model,cam[camID].port);

            syslog(LOG_INFO, "Model = %s",mCameraWrapper[camID]->getModel().c_str());
            syslog(LOG_INFO, "Port = %s",mCameraWrapper[camID]->getPort().c_str());
           // syslog(LOG_INFO, "Summary = %s",mCameraWrapper[camID]->getSummary().c_str());

        }
        catch(...){

            syslog(LOG_ERR, "ERROR IN INIT");

        }
    }


}



bool ShifuCommunicator::readFromClient()
{
    bool result = false;
    ssize_t r;


        uint8_t *buf = (uint8_t *) malloc(sizeof(StructCommuniationPacket));

        r = read(mSocket, buf, sizeof(StructCommuniationPacket));
        syslog(LOG_ERR, "All Read Packet Size r =  : %ld", r);
        if (r >= 2)
        {
            syslog(LOG_ERR, "IN Packet Size r =  : %ld", r);
            result = processPacket(buf, r);
        }

        if(r == -1) result = true; //Handle timeout here



        free(buf);

    syslog(LOG_ERR, "Result before leaving readFromClient r =  : %ld [%d]", r,result);
    return result;
}

void ShifuCommunicator::handleClientConnection(int socket)
{
    mSocket = socket;

    int keep_alive = 1;
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt (mSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
            syslog(LOG_ERR, "RCVTIMEO setsockopt failed");

    if (setsockopt (mSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
            syslog(LOG_ERR, "SNDTIMEO setsockopt failed");
//  if (setsockopt (mSocket, SOL_SOCKET, TCP_USER_TIMEOUT, &keep_alive, sizeof(int)) < 0 )
//       syslog(LOG_ERR, "KEEPALIVE setsockopt failed");
    if (setsockopt(mSocket, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(int)) < 0)
        syslog(LOG_ERR, "KEEPALIVE setsockopt failed");

    // send welcome message
    sendWelcomeMessage();

    while(true) {
        //syslog(LOG_ERR, "Reading from Client ...");
        if (!readFromClient()) {
            syslog(LOG_ERR, "Stoping client");
            break;
        }
    }

    sleep(2);
    close(mSocket);

}


bool ShifuCommunicator::processPacket(uint8_t *buf, int size)
{

    StructCommuniationPacket *packet  = (StructCommuniationPacket *)&buf[0];
    //syslog(LOG_INFO, " packet_command = %x",ntohs(packet->packet_command));
    //syslog(LOG_INFO, " Cam ID = %x",ntohs(packet->camera_number));
    //syslog(LOG_INFO, " widget_type= %x",ntohs(packet->widget_type));
    //syslog(LOG_INFO, " widget_id = %s",packet->widget_id);
    //syslog(LOG_INFO, " widget_value = %s",packet->widget_value);

    //syslog((LOG_INFO)," CAM ID - %d",((int)ntohs(packet->camera_number))-1);

    try {
        switch (ntohs(packet->packet_command)) {
            case GET_CAMERAS_LIST: // 0x0902
            {

                try {
                    uint8_t *returnBuffer = (uint8_t *) malloc((CAM_NAME_MAX_LEN * 4) + (CAM_PORT_MAX_LEN * 4));

                    bzero(&returnBuffer[0], (CAM_NAME_MAX_LEN * 4) + (CAM_PORT_MAX_LEN * 4));

                    int bufferSize = GetConnectedCamerasList(returnBuffer);

                    sendBuffer(returnBuffer, (CAM_NAME_MAX_LEN * 4) + (CAM_PORT_MAX_LEN * 4));
                    free(returnBuffer);
                    //if(bufferSize != 30)
                    //{
                    //    initCameraWrapper();
                    //}
                }catch(...){syslog(LOG_ERR,"Exception while getting camera list!");}

            }
                break;
            case INIT_CAMERA_MODEL: {
                try {
                    initCameraWrapper(((int) ntohs(packet->camera_number)) - 1);
                }catch(...){syslog(LOG_ERR,"Exception while initializing camera!");}
                break;

            }

            case CAPTURE_IMAGE://   0x0002
            {
                try{
                    captureImage(((int) ntohs(packet->camera_number)) - 1);
                }catch(...){syslog(LOG_ERR,"Exception while capturing image!");}
            }
                break;
            case GET_WIDGET_CURRENT_VAL: //0x0007
            {
                try{
                    getWidgetCurrentValue(buf);
                }catch(...){syslog(LOG_ERR,"Exception while Getting current property!");}
            }
                break;

            case GET_WIDGET_POSSIBLE_VALS://0x0008
            {
                try{
                    getWidgetPossibleValues(buf);
                }catch(...){syslog(LOG_ERR,"Exception while getting possible widget values!");}

            }
                break;
            case SET_WIDGET_NEW_VAL: //0x0009
            {
                try {
                    setWidgetNewValue(buf);
                }catch(...){syslog(LOG_ERR,"Exception while setting value!");}
            }
                break;

            case START_LIVE_VIEW: //0x0005
            {
                //syslog(LOG_INFO, "START_LIVE_VIEW packet_command = %x",ntohs(header->packet_command));
                try{
                mIsLiveViewUp[((int) ntohs(packet->camera_number)) - 1] = true;
                }catch(...){syslog(LOG_ERR,"error while live view!");}
            }
                break;

            case STOP_LIVE_VIEW: //0x0006
            {
                try{
                //syslog(LOG_INFO, "STOP_LIVE_VIEW packet_command = %x",ntohs(header->packet_command));
                StopLivePreview(((int) ntohs(packet->camera_number)) - 1);
                }catch(...){syslog(LOG_ERR,"error while stopping live view!");}

            }
                break;
            case DOWNLOAD_IMAGE:  //0x0004

            {
                //Expecting buf as ImageDownloadReqPacket
                try{
                    DownloadFullSizeImage(buf);

                }catch(...) {syslog(LOG_ERR,"error while downloading full image!");}

            }
            break;

            case CLOSE_CONNECTION:   //0x0999
                break;
            default:
                break;
        }

    }catch (...){}

    return true;


}

using namespace std::placeholders;

void ShifuCommunicator::captureImage(int camID)
{
        iAmInCaptureImage[camID] = true; //Pause Live Preview if in progress

        try{
        initCameraWrapper(camID);
        if(mCameraWrapper[camID] == NULL) return;

       /*
        //Expect it from the client for Canon cameras...
        try
        {
            bool capture= true;
            auto captureWidget = mCameraWrapper[camID]->getConfig().getChildByName<gphoto2pp::ToggleWidget>("capture");
            captureWidget.setValue(capture ? 1 : 0);
            mCameraWrapper[camID]->setConfig(captureWidget);

        }
        catch(const std::runtime_error& e)
        {
            // Swallow the exception
            std::cout << "Tried to set canon capture, failed. The camera is probably not a canon" << std::endl;
        }*/


         auto cameraFilePath = mCameraWrapper[camID]->capture(gphoto2pp::CameraCaptureTypeWrapper::Image);
         //mCameraWrapper[camID]->triggerCapture();




             auto cameraFile = mCameraWrapper[camID]->fileGet(cameraFilePath.Folder,
                                                              cameraFilePath.Name,
                                                              gphoto2pp::CameraFileTypeWrapper::Preview);




             mCapturedImageBuffer = cameraFile.getDataAndSize();

             //Kind of go ahead message to the client so it can start reading file in other thread

             ImageDetails imgDetails;
             memset(&imgDetails, 0, sizeof(ImageDetails));

             memcpy(&imgDetails.image_name[0], cameraFilePath.Name.c_str(), cameraFilePath.Name.length() + 1);
             memcpy(&imgDetails.image_folder[0], cameraFilePath.Folder.c_str(), cameraFilePath.Folder.length() + 1);

             //syslog(LOG_INFO,"Image Name: [%s] [%s] ",cameraFilePath.Name.c_str(),imgDetails.image_name);
             //syslog(LOG_INFO,"Image Folder: [%s] [%s]",cameraFilePath.Folder.c_str(),imgDetails.image_folder);

             int stat = write(mSocket, &imgDetails, sizeof(ImageDetails));

        // mCameraWrapper[camID]->SetCameraFree();

        /*
        mCameraWrapper = NULL;
        while(mCameraWrapper == NULL) {
            initCameraWrapper();
        }*/

       /* auto rootWidget = mCameraWrapper->getConfig();
        auto radioWidget = rootWidget.getChildByName<gphoto2pp::ToggleWidget>("viewfinder");
        radioWidget.setValue(1);
        mCameraWrapper->setConfig(rootWidget);
        */
        }
       catch(...)
       {
           syslog(LOG_ERR,"Exception in Capture image!");
       }

        iAmInCaptureImage[camID] = false; //Resume Live Preview if in progress

       //////////////DownloadImageFromFileWrapper(mSocket);
        //cameraFileWrapper.detectMimeType();
        //cameraFileWrapper.adjustNameForMimeType();
        //cameraFileWrapper.save("example4_capture_method_1b."+cameraFileWrapper.getFileName());


}

void ShifuCommunicator::DownloadImageFromFileWrapper(int socket)
{

    try {
        //syslog(LOG_INFO, "In DownloadImageFromFileWrapper");
        std::string strSize = std::to_string(mCapturedImageBuffer.size());

        uint8_t *bufToSend = (uint8_t *) malloc(strSize.length() + 1);
        //syslog(LOG_INFO, "Buf to send...");
        memcpy(&bufToSend[0], strSize.c_str(), strSize.length() + 1);

        // syslog(LOG_INFO, "Captured Image Size = [%s][%d]",strSize.c_str(),strSize.length()+1 );

        int stat = write(socket, bufToSend, strSize.length() + 1);
        if (stat == 0) {}

        free(bufToSend);

        uint16_t confirmationSignal;
        //Read while we get errors that are due to signals.
        do {
            //syslog(LOG_INFO, "Waiting for the confirmation ");
            stat = read(socket, &confirmationSignal, 2);
        } while (stat < 0);

        //Send data through our socket 6666
        stat = write(socket, mCapturedImageBuffer.data(), mCapturedImageBuffer.size());
        // syslog(LOG_INFO, "Write completed... [%d]",stat);
    }catch(...){syslog(LOG_ERR,"Exception in Download image!");}


}

void ShifuCommunicator::DownloadFullSizeImage(uint8_t *buf)
{

    ImageDownloadReqPacket *packet  = (ImageDownloadReqPacket *)&buf[0];

    int camID = ((int)ntohs(packet->camera_number))-1;
    std::string fileName(packet->image_name);
    std::string folderName(packet->image_folder);

    try {

       // syslog(LOG_INFO,"Cam ID : [%d]", camID);
       // syslog(LOG_INFO,"File Name : [%s]", fileName.c_str());
       // syslog(LOG_INFO,"Cam ID : [%s]", folderName.c_str());

        iAmInCaptureImage[camID] = true; //Not really, but as good as reading from disk is making device busy

        auto cameraFile = mCameraWrapper[camID]->fileGet(folderName,
                                                         fileName,
                                                         gphoto2pp::CameraFileTypeWrapper::Normal);


        std::vector<char> imageBuffer = cameraFile.getDataAndSize();

        std::string strSize = std::to_string(imageBuffer.size());

        uint8_t *bufToSend = (uint8_t *) malloc(strSize.length() + 1);
        //syslog(LOG_INFO, "Buf to send...");
        memcpy(&bufToSend[0], strSize.c_str(), strSize.length() + 1);

        //syslog(LOG_INFO, "Captured Image Size = [%s][%d]",strSize.c_str(),strSize.length()+1 );

        int stat = write(mSocket, bufToSend, strSize.length() + 1);
        if (stat == 0) {}

        free(bufToSend);

        uint16_t confirmationSignal;
        //Read while we get errors that are due to signals.
        do {
            //syslog(LOG_INFO, "Waiting for the confirmation ");
            stat = read(mSocket, &confirmationSignal, 2);
        } while (stat < 0);


        stat = write(mSocket, imageBuffer.data(), imageBuffer.size());
        //syslog(LOG_INFO, "Write completed... [%d]",stat);
    }catch(...){ syslog(LOG_ERR,"Exception in downloading full size image!");}

    iAmInCaptureImage[camID] = false;


}

void ShifuCommunicator::StopLivePreview(int camID)
{
    mIsLiveViewUp[camID] = false;

    if(mCameraWrapper[camID] == NULL) return;

    auto rootWidget = mCameraWrapper[camID]->getConfig();
    auto toggleWidget = rootWidget.getChildByName<gphoto2pp::ToggleWidget>("viewfinder");
    toggleWidget.setValue(false);

    mCameraWrapper[camID]->setConfig(rootWidget);
}


bool ShifuCommunicator::SendLiveStreamToClient(int socket) //int socket,struct sockaddr_in clt_addr,unsigned int len)
{
   // syslog(LOG_INFO, "IN SendLiveStreamToClient");

    int keep_alive = 1;
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    int stat;

    try {
        ConfirmationMessage *buf = (ConfirmationMessage *) malloc(sizeof(ConfirmationMessage));

        int camID = 0;

        if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0)
            syslog(LOG_ERR, "RCVTIMEO setsockopt failed");

        if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(timeout)) < 0)
            syslog(LOG_ERR, "SNDTIMEO setsockopt failed");
        //  if (setsockopt (mSocket, SOL_SOCKET, TCP_USER_TIMEOUT, &keep_alive, sizeof(int)) < 0 )
        //       syslog(LOG_ERR, "KEEPALIVE setsockopt failed");
        if (setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(int)) < 0)
            syslog(LOG_ERR, "KEEPALIVE setsockopt failed");

        do {
            // syslog(LOG_INFO, "Reading Confirmation message");
            stat = read(socket, buf, sizeof(ConfirmationMessage));
            // syslog(LOG_INFO, "Read Completed [%d] [%x] [%x]",stat,buf->camera_number,buf->packet_command);
            camID = ((int) (ntohs(buf->camera_number))) - 1;
            //  syslog(LOG_INFO, "Cam ID  = %d" , camID);

        } while (stat < 0);//&& ntohs(buf->packet_command) != 0x4444);


        while (!mIsLiveViewUp[camID])  // Wait till server receives this command from the client
        {
        }
        // syslog(LOG_INFO, "mIsLiveViewUp[%d] = %d",camID,mIsLiveViewUp[camID]);


        initCameraWrapper(camID);
        if (mCameraWrapper[camID] == NULL) return true;

        //syslog(LOG_INFO, "After init");


        gphoto2::_CameraFile *m_cameraFile;
        // uint16_t confirmationSignal;

        gphoto2::gp_file_new(&m_cameraFile);


        while (mIsLiveViewUp[camID]) {
            //syslog(LOG_INFO, "In .. [%d] [%d]",camID,mIsLiveViewUp[camID]);
            if (iAmInCaptureImage[camID]) {
                continue;
            }
            //syslog(LOG_INFO, "iAmInCaptureImage .. [%d]",camID);

            // syslog(LOG_INFO, "wait for go ahead! [%d]",camID);
            do {
                stat = read(socket, buf, sizeof(ConfirmationMessage));

            } while (stat < 0); //&& ntohs(confirmationSignal) != 0x4444);



            //Get Image from camera with size
            // syslog(LOG_INFO, "Just before Capture Preview! [%d]",camID);
            auto buffer = gphoto2pp::helper::capturePreview(*mCameraWrapper[camID]);
            //syslog(LOG_INFO, "Preview Captured!");

            std::string strSize = std::to_string(buffer.size());
            uint8_t *bufToSend = (uint8_t *) malloc(strSize.length() + 1);
            memcpy(&bufToSend[0], strSize.c_str(), strSize.length() + 1);

            stat = write(socket, bufToSend, strSize.length() + 1);

            free(bufToSend);

            //Read while we get errors that are due to signals.
            do {
                //syslog(LOG_INFO, "Waiting for the confirmation [%d]",camID);
                stat = read(socket, buf, sizeof(ConfirmationMessage));
            } while (stat < 0);

            //Send data through our socket
            stat = write(socket, buffer.data(), buffer.size());
            //syslog(LOG_INFO, "Sent stream... [%d]",camID);
        }

        free(buf);

        //initCameraWrapper();

        //mCameraWrapper = NULL;
        /*auto rootWidget = mCameraWrapper->getConfig();
        auto toggleWidget = rootWidget.getChildByName<gphoto2pp::ToggleWidget>("viewfinder");
        toggleWidget.setValue(false);
        mCameraWrapper->setConfig(rootWidget);
        */

        //mCameraWrapper->SetCameraFree();
        //mCameraWrapper = NULL;

        // syslog(LOG_INFO, "OUT SendLiveStreamToClient");
    }catch (...){
        syslog(LOG_ERR,"Exception in Live View streaming!");

    }
    return true;
}


void ShifuCommunicator::getWidgetCurrentValue(uint8_t *buf)
{
    StructCommuniationPacket *packet  = (StructCommuniationPacket *)&buf[0];

    //syslog(LOG_INFO, "getWidgetCurrentValue packet_command = %x",ntohs(packet->packet_command));
    //syslog(LOG_INFO, "getWidgetCurrentValue widget_type= %x",ntohs(packet->widget_type));
    //syslog(LOG_INFO, "getWidgetCurrentValue widget_id = %s",packet->widget_id);
    //syslog(LOG_INFO, "getWidgetCurrentValue widget_value = %s",packet->widget_value);

    int camID = ((int)ntohs(packet->camera_number)) - 1;
    //syslog(LOG_INFO, "getWidgetCurrentVale Cam ID = %d",camID);

    initCameraWrapper(camID);

    if(mCameraWrapper[camID] == NULL) return;

    auto rootWidget = mCameraWrapper[camID]->getConfig();



    std::string wigetID(packet->widget_id);
    std::string strWiget_value;

    switch(ntohs(packet->widget_type))
    {
        case DATE_WIDGET:
        {
            auto dateWidget = rootWidget.getChildByName<gphoto2pp::DateWidget>(wigetID);
            strWiget_value = dateWidget.getValue();
            break;
        }

        case TOGGLE_WIDGET:
        {
            auto toggleWidget = rootWidget.getChildByName<gphoto2pp::ToggleWidget>(wigetID);
            strWiget_value = toggleWidget.getValue();
            break;

        }

        case BUTTON_WIDGET:
        {
            // Need to check this widget function in pp wrapper
            break;
        }

        case RANGE_WIDGET:
        {
            auto rangeWidget = rootWidget.getChildByName<gphoto2pp::RangeWidget>(wigetID);
            strWiget_value = rangeWidget.getValue();
            break;
        }

        case MENU_WIDGET:
        {
            auto menuWidget = rootWidget.getChildByName<gphoto2pp::MenuWidget>(wigetID);
            strWiget_value = menuWidget.getValue();
            break;
        }

        case RADIO_WIDGET:
        {

            auto radioWidget = rootWidget.getChildByName<gphoto2pp::RadioWidget>(wigetID);
            strWiget_value = radioWidget.getValue();

            break;
        }

        case TEXT_WIDGET:
        {
            auto textWidget = rootWidget.getChildByName<gphoto2pp::TextWidget>(wigetID);
            strWiget_value = textWidget.getValue();
            break;
        }

        case SECTION_WIDGET:
        {
            auto sectionWidget = rootWidget.getChildByName<gphoto2pp::NonValueWidget>(wigetID);
            strWiget_value = sectionWidget.countChildren();
            break;
        }
        default:
        {
            syslog(LOG_INFO, "Not a valid option!");
        }


    }
   // syslog(LOG_INFO, ">>> Current strWiget_value = %s",strWiget_value);



    std::string strSize  = std::to_string(strWiget_value.length()+1);
    uint8_t *SizeBufToSend = (uint8_t *)malloc(strSize.length()+1);
    memcpy(&SizeBufToSend[0], strSize.c_str(), strSize.length()+1);
    sendBuffer(SizeBufToSend,strSize.length()+1);

    free(SizeBufToSend);

    //Read Confirmation
    int stat;
    ConfirmationMessage *confirmationMessage = (ConfirmationMessage *) malloc(sizeof(ConfirmationMessage));

    do {
        stat=read(mSocket, confirmationMessage , sizeof(ConfirmationMessage));

    } while (stat < 0 ); //&& ntohs(confirmationSignal) != 0x4444);

    free(confirmationMessage);

    uint8_t *bufToSend = (uint8_t *)malloc(strWiget_value.length()+1);
    memcpy(&bufToSend[0], strWiget_value.c_str(), strWiget_value.length()+1);
    sendBuffer(bufToSend,strWiget_value.length()+1);

    free(bufToSend);


}
void ShifuCommunicator::getWidgetPossibleValues(uint8_t *buf)
{

    StructCommuniationPacket *packet  = (StructCommuniationPacket *)&buf[0];

    //syslog(LOG_INFO, "getWidgetCurrentValue packet_command = %x",ntohs(packet->packet_command));
    //syslog(LOG_INFO, "getWidgetCurrentValue widget_type= %x",ntohs(packet->widget_type));
    //syslog(LOG_INFO, "getWidgetCurrentValue widget_id = %s",packet->widget_id);
    //syslog(LOG_INFO, "getWidgetCurrentValue widget_value = %s",packet->widget_value);

    int camID = ((int)ntohs(packet->camera_number) )- 1;

    initCameraWrapper(camID);
    if(mCameraWrapper[camID] == NULL) return;

    auto rootWidget = mCameraWrapper[camID]->getConfig();

    std::string wigetID(packet->widget_id);
    std::string strWiget_PossibleValues;

    switch(ntohs(packet->widget_type))
    {
        case DATE_WIDGET:
        {
            //auto dateWidget = rootWidget.getChildByName<gphoto2pp::DateWidget>(wigetID);
            //strWiget_value = dateWidget.getValue();
            break;
        }

        case TOGGLE_WIDGET:
        {
            //auto toggleWidget = rootWidget.getChildByName<gphoto2pp::ToggleWidget>(wigetID);
            //strWiget_value = toggleWidget.getValue();
            break;

        }

        case BUTTON_WIDGET:
        {
            // Need to check this widget function in pp wrapper
            break;
        }

        case RANGE_WIDGET:
        {
            auto rangeWidget = rootWidget.getChildByName<gphoto2pp::RangeWidget>(wigetID);
            strWiget_PossibleValues = rangeWidget.ToString();
            break;
        }

        case MENU_WIDGET:
        {
            auto menuWidget = rootWidget.getChildByName<gphoto2pp::MenuWidget>(wigetID);
            strWiget_PossibleValues = menuWidget.choicesToString(" ");
            break;
        }

        case RADIO_WIDGET:
        {

            auto radioWidget = rootWidget.getChildByName<gphoto2pp::RadioWidget>(wigetID);
            strWiget_PossibleValues = radioWidget.choicesToString(" ");

            break;
        }

        case TEXT_WIDGET:
        {
            //auto textWidget = rootWidget.getChildByName<gphoto2pp::TextWidget>(wigetID);
            //strWiget_PossibleValues = textWidget.getValue();
            break;
        }

        case SECTION_WIDGET:
        {
            auto sectionWidget = rootWidget.getChildByName<gphoto2pp::NonValueWidget>(wigetID);
            strWiget_PossibleValues = sectionWidget.countChildren();
            break;
        }
        default:
        {
            syslog(LOG_INFO, "Not a valid option!");
        }


    }
    //syslog(LOG_INFO, ">>> Current strWiget_value = %s",strWiget_value);


        //Send Data Size
        std::string strSize  = std::to_string(strWiget_PossibleValues.length()+1);
        uint8_t *SizeBufToSend = (uint8_t *)malloc(strSize.length()+1);
        memcpy(&SizeBufToSend[0], strSize.c_str(), strSize.length()+1);
        int stat = write(mSocket,SizeBufToSend,strSize.length()+1);
        //sendBuffer(SizeBufToSend,strSize.length()+1);

        free(SizeBufToSend);

        syslog(LOG_INFO,"getWidgetPossibleValues: Size Sent : [%s]  Stat = [%d]", strSize.c_str(), stat);

        //Read Confirmation
        ConfirmationMessage *confirmationMessage = (ConfirmationMessage *) malloc(sizeof(ConfirmationMessage));

        do {
                stat=read(mSocket, confirmationMessage , sizeof(ConfirmationMessage));
                syslog(LOG_INFO,"getWidgetPossibleValues: Reading Confirmation : %x", ntohs(confirmationMessage->packet_command));

        } while (stat < 0 ); //&& ntohs(confirmationMessage) != 0x4444);

        free(confirmationMessage);

        //Send Data
        uint8_t *bufToSend = (uint8_t *)malloc(strWiget_PossibleValues.length()+1);
        memcpy(&bufToSend[0], strWiget_PossibleValues.c_str(), strWiget_PossibleValues.length()+1);
        sendBuffer(bufToSend,strWiget_PossibleValues.length()+1);

        free(bufToSend);

        syslog(LOG_INFO,"getWidgetPossibleValues: Data Sent");


}
void ShifuCommunicator::setWidgetNewValue(uint8_t *buf)
{
    StructCommuniationPacket *packet  = (StructCommuniationPacket *)&buf[0];

    syslog(LOG_INFO, "getWidgetCurrentValue packet_command = %x",ntohs(packet->packet_command));
    syslog(LOG_INFO, "getWidgetCurrentValue widget_type= %x",ntohs(packet->widget_type));
    syslog(LOG_INFO, "getWidgetCurrentValue widget_id = %s",packet->widget_id);
    syslog(LOG_INFO, "getWidgetCurrentValue widget_value = %s",packet->widget_value);

    int camID = ((int)ntohs(packet->camera_number)) - 1;

    initCameraWrapper(camID);
    if(mCameraWrapper[camID] == NULL) return;

    auto rootWidget = mCameraWrapper[camID]->getConfig();

    std::string wigetID(packet->widget_id);
    std::string strWiget_value;

    std::string wigetValue(packet->widget_value);


    try {
        switch (ntohs(packet->widget_type)) {
            case DATE_WIDGET: {
                auto dateWidget = rootWidget.getChildByName<gphoto2pp::DateWidget>(wigetID);
                dateWidget.setValue(std::stol(wigetValue));
                mCameraWrapper[camID]->setConfig(rootWidget);
                break;
            }

            case TOGGLE_WIDGET: {
                auto toggleWidget = rootWidget.getChildByName<gphoto2pp::ToggleWidget>(wigetID);

                bool val = (wigetValue.compare("true") == 0) ? true : false;

                toggleWidget.setValue(val);

                mCameraWrapper[camID]->setConfig(rootWidget);
                break;

            }

            case BUTTON_WIDGET: {
                // Need to check this widget function in pp wrapper
                break;
            }

            case RANGE_WIDGET: {
                auto rangeWidget = rootWidget.getChildByName<gphoto2pp::RangeWidget>(wigetID);
                rangeWidget.setValue(std::stoi(wigetValue));
                mCameraWrapper[camID]->setConfig(rootWidget);
                break;
            }

            case MENU_WIDGET: {

                auto menuWidget = rootWidget.getChildByName<gphoto2pp::MenuWidget>(wigetID);
                menuWidget.setValue(wigetValue);
                mCameraWrapper[camID]->setConfig(rootWidget);
                break;
            }

            case RADIO_WIDGET: {
                syslog(LOG_INFO,"[%s] [%s]",wigetID.c_str(),wigetValue.c_str());

                auto radioWidget = rootWidget.getChildByName<gphoto2pp::RadioWidget>(wigetID);
                radioWidget.setValue(wigetValue);
                mCameraWrapper[camID]->setConfig(rootWidget);

                break;
            }


            case TEXT_WIDGET: {
                //Use this to set Control Mode only, else your server will crash
                auto textWidget = rootWidget.getChildByName<gphoto2pp::TextWidget>(wigetID);
                textWidget.setValue(wigetValue);
                mCameraWrapper[camID]->setConfig(rootWidget);

                break;
            }

            case SECTION_WIDGET: {
                //Nothing to do
                break;
            }
            default: {
                syslog(LOG_INFO, "Not a valid option!");
            }


        }

    }catch(...)
    {

    }
        getWidgetCurrentValue(buf); // Send modified value back to the client - Reusing this function here.

}


int ShifuCommunicator::GetConnectedCamerasList(uint8_t *buf)
{
    int nCameraCount =0;
    int offset = 0;
    int requriedBufferLen = 0;

    try
    {	//std::cout << "IN";
        auto cameraList = gphoto2pp::autoDetectAll();


        //std::cout << "1";
        nCameraCount = cameraList.count();

        requriedBufferLen = CAM_NAME_MAX_LEN * nCameraCount;
        //std::cout << "2";
        //Considering max 4 cameras can be connected to the device & each would have name length aroud 50 bytes. (Just to keep it simple)

        //buf = (uint8_t *)malloc(requriedBufferLen);

        //[50 : Cam 1 Model][15: Cam1 Port][50:Cam2 Model][15:Cam2 Port]...[15:Cam4 Port]
        for(int i = 0; i < nCameraCount; i++)
        {
            std::cout << i<< ") Name:Port: " << cameraList.getName(i) <<":"<<cameraList.getValue(i) << std::endl;

            std::string camName = cameraList.getName(i);
            std::string camPort = cameraList.getValue(i);

            memcpy(&buf[offset], camName.data(), camName.length());

            offset += (CAM_NAME_MAX_LEN+1);

            //Prepare Server struct to keep connected cameras list
            cam[i].model = camName;
            cam[i].port  = camPort;

            //memcpy(&buf[offset], camPort.data(), camPort.length());
            //offset += (CAM_PORT_MAX_LEN+1);

        }
    }
    catch (const gphoto2pp::exceptions::NoCameraFoundError &e)
    {
        requriedBufferLen  = 30;
        buf = (uint8_t *)malloc(requriedBufferLen);

        std::string errorMessage = "No Connected device found!";

        strcpy((char *)&buf[0], errorMessage.c_str());

        free(buf);

        std::cout << "GetConnectedCamerasList Exception Message: " << e.what() << std::endl;
    }


    return requriedBufferLen;

}












