////////////////////////////////////////////////////////////////////////////////
/**
 * @file    remote.cpp
 * @author  Federico Roux (federico.roux@globant.com)
 * @brief   Remote App of C++ code to create a remote downstream.
 * @version 0.1
 * @date    2022-11-23
 */
////////////////////////////////////////////////////////////////////////////////
/**
 * @author  Pedro Shinyashiki (pedro.shinyashiki@globant.com)
 * @brief   First Remote Gstreamer Pipeline C++
 * @version 0.2
 * @date    2022-12-07
 * */
////////////////////////////////////////////////////////////////////////////////
/**
 * @author  Pedro Shinyashiki (pedro.shinyashiki@globant.com)
 * @brief   Extract Images from the video stream and send to a socket using appsink
 * @version 0.3
 * @date    2023-02-23
 * */
////////////////////////////////////////////////////////////////////////////////
#define VERSION 0.3
////////////////////////////////////////////////////////////////////////////////
/*
How To test:
1) Run gstreamer-remote (this app)
2) Run socket client: python3 socket_client.py
3) Run the gstreamer script:
  FILE=Legend.mp4; REMOTE_IP=127.0.0.1; PORT=4000;
  gst-launch-1.0 -v filesrc location = $FILE ! decodebin ! x264enc ! rtph264pay ! udpsink host=$REMOTE_IP port=$PORT
4) You will see JPG files numbered in the same folder where is running the python script
5) Enjoy
////////////////////////////////////////////////////////////////////////////////
Scripts:
Local:
FILE=Legend.mp4; REMOTE_IP=127.0.0.1; PORT=4000;
gst-launch-1.0 -v filesrc location = $FILE ! decodebin ! x264enc ! rtph264pay ! udpsink host=$REMOTE_IP port=$PORT
Remote:
gst-launch-1.0 udpsrc port={self._port} ! application/x-rtp, encoding-name=H264, payload=96  ! \
                              rtph264depay ! avdec_h264 ! autovideoconvert ! appsink  emit-signals=True
*/
////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <string>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>   //For appsink
#include <utils.h>
#include <thread>                 //For thread
#include <iomanip>                //For setfill
#include <sstream>                //For stringstream
#include <vector>                 //For vector
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>               //For write
////////////////////////////////////////////////////////////////////////////////
#define SERVER_PORT_DATA      4008 //htons??
#define SERVER_PORT_HANDSHAKE 4008
////////////////////////////////////////////////////////////////////////////////

typedef struct {
  GstElement *pipeline;
  GstElement *source;
  GstElement *enc_img;
  GstElement *sink;
  GstElement *conv;
  GstElement *h264dec;
  GstElement *rtp_dec;
  GstCaps    *filtercaps;
} pipeline_t;

pipeline_t p;                       //Accessed by the thread
std::vector<uint8_t> appsinkbuffer;
std::vector<uint32_t> buffersize;

pthread_mutex_t mtx_buff;
std::thread appsink_thread, socket_thread;
bool m_isRunning = true;

////////////////////////////////////////////////////////////////////////////////
int main (int argc, char *argv[])
{
  std::cout << "**************************************************"<< std::endl;
  std::cout << "*************** Remote App v. " << VERSION << " ****************" << std::endl;
  std::cout << "**************************************************"<< std::endl;

  
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;

  gint port = 0;

  if(argc <= 1) {
    port = 4000;
  }
  else {
    port = std::stoi(argv[1]);
  }

  if(utils::validate_port(port)) {
    std::cout << "Running on Port: " << port << std::endl;
  }
  else {
    std::cout << "Not valid port. Exiting..." << std::endl;
    exit(EXIT_FAILURE);
  }

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Create the elements */
  /*
  gst-launch-1.0 udpsrc port=PORT ! application/x-rtp, encoding-name=H264, payload=96 ! \
              rtph264depay ! avdec_h264 ! autovideoconvert ! autovideosink
  */
  p.source = gst_element_factory_make ("udpsrc", "source");
  ASSERT_ELEMENT(p.source, "udpsrc");
  p.filtercaps = gst_caps_new_simple("application/x-rtp",
    //"media", G_TYPE_STRING, "video",
    "payload", G_TYPE_INT, 96,
    //"clock-rate", G_TYPE_INT, 90000,
    "encoding-name", G_TYPE_STRING, "H264", NULL);
  ASSERT_ELEMENT(p.filtercaps, "filtercaps");
  g_object_set(G_OBJECT(p.source), "caps", p.filtercaps, NULL);
  gst_caps_unref(p.filtercaps);
  p.rtp_dec = gst_element_factory_make("rtph264depay", "rtp_dec");
  ASSERT_ELEMENT(p.rtp_dec, "rtph264depay");
  p.h264dec = gst_element_factory_make("avdec_h264", "dec");
  ASSERT_ELEMENT(p.h264dec, "avdec_h264");
  p.conv = gst_element_factory_make("autovideoconvert", "conv");
  ASSERT_ELEMENT(p.conv, "autovideoconvert");

  p.enc_img = gst_element_factory_make("jpegenc", "enc");
  ASSERT_ELEMENT(p.enc_img, "jpegenc");

  // p.enc_img = gst_element_factory_make("pngenc", "enc");
  // ASSERT_ELEMENT(p.enc_img, "pngenc");

  // //Write directly to a file
  // p.sink = gst_element_factory_make("multifilesink", "sink");
  // ASSERT_ELEMENT(p.sink, "multifilesink");
  // g_object_set(p.sink, "location", "out-%05d.jpg", NULL);

  //Appsink
  p.sink = gst_element_factory_make("appsink", "extract_images_appsink");
  ASSERT_ELEMENT(p.sink, "appsink"); // Checks if NULL
  g_object_set (G_OBJECT (p.sink), "emit-signals", FALSE, "sync", FALSE, NULL);

////////////////////////////////////////////////////////////////////////////////
  // Thread to read from the appsink buffer
  try
  {
    appsink_thread = std::thread([]() 
    {
      int filecount = 0;
      std::cout << "------ START AppSink Thread ------" << std::endl;
      while (m_isRunning)
      {
        GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK_CAST(p.sink));
        if(sample != NULL)
        {
          GstBuffer * buffer  = gst_sample_get_buffer (sample);
          if (buffer == NULL)
          {
            continue;
          }
          gsize datalen = gst_buffer_get_size(buffer);
          // Extract buffer
          uint8_t *rawData = (uint8_t *) malloc(datalen);
          if (rawData == NULL) {
            std::cout << " build_pipeline() allocation fails" << std::endl;
            return false;
          }
          memset(rawData, 0, datalen);
          gst_buffer_extract(buffer, 0, rawData, datalen);
          //Append to a local buffer
          pthread_mutex_lock(&mtx_buff);
          appsinkbuffer.insert( appsinkbuffer.end(), rawData, rawData + datalen );
          buffersize.push_back(datalen);
          pthread_mutex_unlock(&mtx_buff);
          ////////////////////////////////////
          // //Write to files
          // FILE* pFile;
          // std::stringstream filename;
          // std::string fname;
          // filename << "file_" << std::setw(2) << std::setfill('0') << filecount << ".raw";
          // fname = std::string (filename.str());
          // pFile = fopen(fname.c_str(), "wb");
          // fwrite(rawData, datalen, 1, pFile);
          // fclose(pFile);
          ////////////////////////////////////
          free(rawData);
          gst_sample_unref(sample);
          filecount++;
        }
      }
      std::cout << "------ END AppSink Thread ------" << std::endl;
      m_isRunning=false;
      return true;
    });

    appsink_thread.detach();
  }
  catch (std::exception &e)
  {
      std::cout << "Appsink - thread start error: " << e.what() << std::endl;
      std::cout << "------ Substhread start error ------" << std::endl;
  }
////////////////////////////////////////////////////////////////////////////////
  // Thread to handle the socket and send frames
  try
  {
    socket_thread = std::thread([]() 
    {
      //Socket
      int server_socket, client_fd;
      struct sockaddr_in server_addr, client_addr;

      // Creating socket file descriptor
      // if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      if ((server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
          perror("socket failed");
          exit(EXIT_FAILURE);
      }
      
      server_addr.sin_family = AF_INET;
      server_addr.sin_port = htons(SERVER_PORT_DATA);
      server_addr.sin_addr.s_addr = INADDR_ANY;

      // if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
      //     perror("bind failed");
      //     exit(EXIT_FAILURE);
      // }
      // if (listen(server_socket, 1) < 0) {
      //     perror("listen");
      //     exit(EXIT_FAILURE);
      // }

      // socklen_t sin_size=sizeof(client_addr);
      // client_fd=accept(server_socket,(struct sockaddr*)&client_addr, &sin_size);
      // //std::cout << "client_fd:" << client_fd << std::endl;

      // if (client_fd < 0) {
      //   perror("accept");
      //   exit(EXIT_FAILURE);
      // };

      client_fd = server_socket;
      printf("Got connection from %s port %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

      int filecount2 = 0;
      std::cout << "------ START Socket Thread ------" << std::endl;
      uint32_t datalen2=0;
      uint8_t *rawData2;
      while (true){
        datalen2=0;
        //////////////////////////////
        //Extract from a local buffer                                                                                                                    
        pthread_mutex_lock(&mtx_buff);
        if ( ! buffersize.empty()) {
          datalen2=buffersize.front();
          buffersize.erase(buffersize.begin(),buffersize.begin()+1);
        }
        if (datalen2 != 0) {
          rawData2 = (uint8_t *) malloc(datalen2);
          memset(rawData2, 0, datalen2);
          rawData2=appsinkbuffer.data();
          std::cout << "[Socket Thread] frame:" << filecount2 << " lenght: " << datalen2 << std::endl;
          appsinkbuffer.erase( appsinkbuffer.begin(), appsinkbuffer.begin() + datalen2 );
          //Send Frame Number, Frame Lenght and Frame
          sendto(client_fd, &(filecount2), sizeof(filecount2),0, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr));
          sendto(client_fd, &(datalen2), sizeof(datalen2),0, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr));
          sendto(client_fd, rawData2, datalen2, 0, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr));
          //std::cout << "Sent: Frame: "<< filecount2 << "[" << sizeof(filecount2) << "] with lenght: " << datalen2 << "[" << sizeof(datalen2) << "]" << std::endl;
          filecount2++;
        }
        pthread_mutex_unlock(&mtx_buff);
        //////////////////////////////  
        // //Write to files
        // FILE* pFile;
        // std::stringstream filename;
        // std::string fname;
        // filename << "file_" << std::setw(2) << std::setfill('0') << filecount2 << ".jpg";
        // fname = std::string (filename.str());
        // pFile = fopen(fname.c_str(), "wb");
        // fwrite(rawData2, datalen2, 1, pFile);
        // fclose(pFile);
        //////////////////////////////  
        g_usleep(100000);
      }
      std::cout << "------ END Socket Thread ------" << std::endl;
      return true;
    });
    socket_thread.detach();
  }
  catch (std::exception &e)
  {
      std::cout << "Socket - thread start error: " << e.what() << std::endl;
      std::cout << "------ Substhread start error ------" << std::endl;
  }
////////////////////////////////////////////////////////////////////////////////
  /* Create the empty pipeline */
  p.pipeline = gst_pipeline_new ("test-pipeline");

  if (!p.pipeline || !p.source || !p.rtp_dec || !p.h264dec || !p.conv || !p.enc_img || !p.sink) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }

  /* Build the pipeline */
  gst_bin_add_many (GST_BIN (p.pipeline), p.source, p.rtp_dec, p.h264dec, p.conv, p.enc_img, p.sink, NULL);
  
  if (gst_element_link_many (p.source, p.rtp_dec, NULL) != TRUE) {
    g_printerr ("First Elements could not be linked.\n");
    gst_object_unref (p.pipeline);
    return -1;
  }

  if (gst_element_link_many (p.rtp_dec, p.h264dec, p.conv, p.enc_img, p.sink, NULL) != TRUE) {
    g_printerr ("Next Elements could not be linked.\n");
    gst_object_unref (p.pipeline);
    return -1;
  }

  /* Set udpsink ip and port */
  g_object_set (p.source, "port", static_cast<gint>(port), NULL);

  /* Start playing */
  ret = gst_element_set_state (p.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (p.pipeline);
    return -1;
  }

  /* Wait until error or EOS */
  bus = gst_element_get_bus (p.pipeline);
  msg =
      gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

  /* Parse message */
  if (msg != NULL) {
    GError *err;
    gchar *debug_info;

    switch (GST_MESSAGE_TYPE (msg)) {
      case GST_MESSAGE_ERROR:
        gst_message_parse_error (msg, &err, &debug_info);
        g_printerr ("Error received from element %s: %s\n",
            GST_OBJECT_NAME (msg->src), err->message);
        g_printerr ("Debugging information: %s\n",
            debug_info ? debug_info : "none");
        g_clear_error (&err);
        g_free (debug_info);
        break;
      case GST_MESSAGE_EOS:
        g_print ("End-Of-Stream reached.\n");
        break;
      default:
        /* We should not reach here because we only asked for ERRORs and EOS */
        g_printerr ("Unexpected message received.\n");
        break;
    }
    gst_message_unref (msg);
  }

  /* Free rep.sources */
  gst_object_unref (bus);
  gst_element_set_state (p.pipeline, GST_STATE_NULL);
  gst_object_unref (p.pipeline);
  return 0;
}
////////////////////////////////////////////////////////////////////////////////
