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
 * @brief   Extract Images from the video stream and send to a socket
 * @version 0.3
 * @date    2022-12-27
 * */
////////////////////////////////////////////////////////////////////////////////
#define VERSION 0.3
////////////////////////////////////////////////////////////////////////////////
/*
Scripts:
Local:
gst-launch-1.0 -v filesrc location = $FILE ! decodebin ! x264enc ! rtph264pay ! udpsink host=$REMOTE_IP port=$PORT
Remote:
gst-launch-1.0 udpsrc port={self._port} ! application/x-rtp, encoding-name=H264, payload=96  ! \
                              rtph264depay ! avdec_h264 ! autovideoconvert ! appsink  emit-signals=True
*/
/*
rtpjpegdepay ! \
 jpegdec ! \
 autovideosink
 */
////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <string>
#include <gst/gst.h>
#include <utils.h>
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

////////////////////////////////////////////////////////////////////////////////
int main (int argc, char *argv[])
{
  std::cout << "**************************************************"<< std::endl;
  std::cout << "*************** Remote App v. " << VERSION << " ****************" << std::endl;
  std::cout << "**************************************************"<< std::endl;

  pipeline_t p;
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

  p.sink = gst_element_factory_make("multifilesink", "sink");
  ASSERT_ELEMENT(p.sink, "multifilesink");
  g_object_set(p.sink, "location", "out-%05d.jpg", NULL);

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
