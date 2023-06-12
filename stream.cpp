#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <iostream>
#include <sstream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <zmqpp/zmqpp.hpp>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace cv;

zmqpp::message createMessage(const cv::Mat &frame)
{
    // Convert the frame to binary data
    std::vector<uchar> buffer;
    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 50}; // Set JPEG quality to 50
    cv::imencode(".jpg", frame, buffer, params);

    // Create a ZMQ message from the binary data
    zmqpp::message message(buffer.data(), buffer.size());

    return message;
}

int PrintDeviceInfo(INodeMap &nodeMap)
{
    int result = 0;

    std::cout << std::endl
              << "*** DEVICE INFORMATION ***" << std::endl
              << std::endl;

    try
    {
        FeatureList_t features;
        CCategoryPtr category = nodeMap.GetNode("DeviceInformation");
        if (IsReadable(category))
        {
            category->GetFeatures(features);

            FeatureList_t::const_iterator it;
            for (it = features.begin(); it != features.end(); ++it)
            {
                CNodePtr pfeatureNode = *it;
                std::cout << pfeatureNode->GetName() << " : ";
                CValuePtr pValue = (CValuePtr)pfeatureNode;
                std::cout << (IsReadable(pValue) ? pValue->ToString() : "Node not readable");
                std::cout << std::endl;
            }
        }
        else
        {
            std::cout << "Device control information not readable." << std::endl;
        }
    }
    catch (Spinnaker::Exception &e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        result = -1;
    }

    return result;
}

int main(int argc, char **argv)
{
    int result = 0;

    // Initialize ZMQ context and socket
    zmqpp::context context;
    zmqpp::socket_type type = zmqpp::socket_type::pub;
    zmqpp::socket socket(context, type);
    socket.bind("tcp://*:5555");

    // Retrieve singleton reference to system object
    SystemPtr system = System::GetInstance();

    // Retrieve list of cameras from the system
    CameraList camList = system->GetCameras();

    // Exit if no camera is found
    if (camList.GetSize() == 0)
    {
        std::cout << "No camera detected. Exiting..." << std::endl;
        camList.Clear();
        system->ReleaseInstance();
        return -1;
    }

    // Use first available camera
    CameraPtr pCam = camList.GetByIndex(0);

    try
    {
        // Initialize camera
        pCam->Init();
        INodeMap &nodeMap = pCam->GetNodeMap();

        PrintDeviceInfo(nodeMap);

        // CEnumerationPtr ptrPixelFormat = nodeMap.GetNode("PixelFormat");

        // CEnumEntryPtr ptrPixelFormatBayerRG8 = ptrPixelFormat->GetEntryByName("BayerRG8");
        // if (!IsAvailable(ptrPixelFormatBayerRG8) || !IsReadable(ptrPixelFormatBayerRG8))
        // {
        //     CEnumEntryPtr ptrPixelFormatMono8 = ptrPixelFormat->GetEntryByName("Mono8");
        //     if (!IsAvailable(ptrPixelFormatMono8) || !IsReadable(ptrPixelFormatMono8))
        //     {
        //         std::cout << "Unable to set pixel format to BayerRG8 or Mono8. Aborting..." << std::endl << std::endl;
        //         return false;
        //     }
        //     ptrPixelFormat->SetIntValue(ptrPixelFormatMono8->GetValue());
        //     std::cout << "Pixel format set to " << ptrPixelFormatMono8->GetSymbolic() << "..." << std::endl;
        // }
        // else
        // {
        //     ptrPixelFormat->SetIntValue(ptrPixelFormatBayerRG8->GetValue());
        //     std::cout << "Pixel format set to " << ptrPixelFormatBayerRG8->GetSymbolic() << "..." << std::endl;
        // }

        // Get the TLStream.StreamBufferHandlingMode node
        CEnumerationPtr ptrStreamBufferHandlingMode = pCam->GetTLStreamNodeMap().GetNode("StreamBufferHandlingMode");

        if (!IsAvailable(ptrStreamBufferHandlingMode) || !IsWritable(ptrStreamBufferHandlingMode))
        {
            std::cout << "Unable to set StreamBufferHandlingMode (node retrieval). Aborting..." << std::endl;
            return -1;
        }

        ptrStreamBufferHandlingMode->SetIntValue(StreamBufferHandlingMode_NewestOnly);

        // // Set the handling mode to "Newest Only"
        // if (ptrStreamBufferHandlingMode->SetIntValue(StreamBufferHandlingMode_NewestOnly) < 0)
        // {
        //     std::cout << "Unable to set StreamBufferHandlingMode (value setting). Aborting..." << std::endl;
        //     return -1;
        // }

        // Get the Balance White Auto enumeration node
        CEnumerationPtr ptrBalanceWhiteAuto = pCam->GetNodeMap().GetNode("BalanceWhiteAuto");

        // Check if the node is available and writable
        if (IsAvailable(ptrBalanceWhiteAuto) && IsWritable(ptrBalanceWhiteAuto))
        {
            // Get the Continuous entry node
            CEnumEntryPtr ptrBalanceWhiteAutoContinuous = ptrBalanceWhiteAuto->GetEntryByName("Continuous");

            // Check if the entry node is available and readable
            if (IsAvailable(ptrBalanceWhiteAutoContinuous) && IsReadable(ptrBalanceWhiteAutoContinuous))
            {
                // Set Balance White Auto to Continuous mode
                int64_t balanceWhiteAutoContinuous = ptrBalanceWhiteAutoContinuous->GetValue();
                ptrBalanceWhiteAuto->SetIntValue(balanceWhiteAutoContinuous);
            }
            else
            {
                std::cout << "Unable to set auto white balance to continuous mode (entry 'continuous' retrieval). Aborting..." << std::endl;
                pCam->DeInit();
                pCam = nullptr;
                camList.Clear();
                system->ReleaseInstance();
                return -1;
            }
        }
        else
        {
            std::cout << "Unable to set auto white balance (node retrieval). Aborting..." << std::endl;
            pCam->DeInit();
            pCam = nullptr;
            camList.Clear();
            system->ReleaseInstance();
            return -1;
        }

        // Retrieve the gain auto node
        CEnumerationPtr ptrGainAuto = nodeMap.GetNode("GainAuto");
        if (!IsAvailable(ptrGainAuto) || !IsWritable(ptrGainAuto))
        {
            std::cout << "Unable to set gain auto to continuous (node retrieval). Aborting..." << std::endl;
            return -1;
        }

        // Set gain auto to continuous
        CEnumEntryPtr ptrGainAutoContinuous = ptrGainAuto->GetEntryByName("Continuous");
        if (!IsAvailable(ptrGainAutoContinuous) || !IsReadable(ptrGainAutoContinuous))
        {
            std::cout << "Unable to set gain auto to continuous (entry 'continuous' retrieval). Aborting..." << std::endl;
            return -1;
        }

        int64_t gainAutoContinuous = ptrGainAutoContinuous->GetValue();
        ptrGainAuto->SetIntValue(gainAutoContinuous);

        // Set acquisition mode to continuous
        CEnumerationPtr ptrAcquisitionMode = pCam->GetNodeMap().GetNode("AcquisitionMode");
        if (!IsAvailable(ptrAcquisitionMode) || !IsWritable(ptrAcquisitionMode))
        {
            std::cout << "Unable to set acquisition mode to continuous (node retrieval). Aborting..." << std::endl;
            pCam->DeInit();
            pCam = nullptr;
            camList.Clear();
            system->ReleaseInstance();
            return -1;
        }

        CEnumEntryPtr ptrAcquisitionModeContinuous = ptrAcquisitionMode->GetEntryByName("Continuous");
        if (!IsAvailable(ptrAcquisitionModeContinuous) || !IsReadable(ptrAcquisitionModeContinuous))
        {
            std::cout << "Unable to set acquisition mode to continuous (entry 'continuous' retrieval). Aborting..." << std::endl;
            pCam->DeInit();
            pCam = nullptr;
            camList.Clear();
            system->ReleaseInstance();
            return -1;
        }

        int64_t acquisitionModeContinuous = ptrAcquisitionModeContinuous->GetValue();
        ptrAcquisitionMode->SetIntValue(acquisitionModeContinuous);

        pCam->BeginAcquisition();

        std::cout << "Begin acquiring images..." << std::endl;

        Mat cvImage;

        while (1)
        {
            // Retrieve next received image and ensure image completion
            ImagePtr pResultImage = pCam->GetNextImage(1000);

            if (pResultImage->IsIncomplete())
            {
                std::cout << "Image incomplete with image status " << pResultImage->GetImageStatus() << "..." << std::endl;
            }
            else
            {
                // Convert the Spinnaker image to a suitable pixel format for a colored image
                ImageProcessor processor;
                processor.SetColorProcessing(SPINNAKER_COLOR_PROCESSING_ALGORITHM_HQ_LINEAR);
                ImagePtr convertedImage = processor.Convert(pResultImage, PixelFormat_BGR8);

                unsigned int rowsize = convertedImage->GetWidth();
                unsigned int colsize = convertedImage->GetHeight();
                cvImage = Mat(colsize, rowsize, CV_8UC3, convertedImage->GetData(), convertedImage->GetStride());
                // Serialize the Mat into a byte buffer
                std::vector<uchar> buffer;
                std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 50}; // Set JPEG quality to 50
                cv::imencode(".jpg", cvImage, buffer, params);

                // Create a ZMQ message from the byte buffer
                zmqpp::message message;
                message.add_raw(buffer.data(), buffer.size());
                socket.send(message);
            }

            pResultImage->Release();

            // Press 'q' to exit the loop
            if (waitKey(30) == 'q')
            {
                break;
            }
        }

        pCam->EndAcquisition();
        pCam->DeInit();

        // Release camera and system
        pCam = nullptr;
        camList.Clear();
        system->ReleaseInstance();
    }
    catch (Spinnaker::Exception &e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        result = -1;
    }

    return result;
}
