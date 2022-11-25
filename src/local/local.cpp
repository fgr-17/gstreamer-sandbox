/**
 * @file local.cpp
 * @author  Federico Roux (federico.roux@globant.com)
 * @brief   Basic example of C++ code to create a local upstream.
 * @version 0.1
 * @date 2022-11-23
 * 
 */

#include <iostream>
#include <string>
#include <gst/gst.h>
#include <utils.h>

/**
 * @brief main app
 * 
 * @param argc 
 * @param argv 1st parameter is optional remote IP
 * @return int 
 */

int main (int argc, char *argv[])
{
  GstElement *pipeline, *source, *sink, *filter, *converter;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;

  std::string remote_ip{"0.0.0.0"};
  gint port = 0;

  if(argc <= 2) {
    remote_ip = "127.0.0.1";
    port = 4000;
  }
  else {
    remote_ip = argv[1];
    port = std::stoi(argv[2]);
  }

  if(utils::validate_ip(remote_ip)) {
    std::cout << "Remote IP: " << remote_ip << std::endl;
  }
  else {
    std::cout << "Not valid IP. Exiting..." << std::endl;
    exit(EXIT_FAILURE);
  }

  if(utils::validate_port(port)) {
    std::cout << "Port: " << port << std::endl;
  }
  else {
    std::cout << "Not valid port. Exiting..." << std::endl;
    exit(EXIT_FAILURE);
  }

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Create the elements */
  source = gst_element_factory_make ("videotestsrc", "source");
  filter = gst_element_factory_make("vertigotv", "filter");
  converter = gst_element_factory_make("videoconvert", "converter");
  // sink = gst_element_factory_make ("autovideosink", "sink");
  sink = gst_element_factory_make("udpsink", "sink");


  /* Create the empty pipeline */
  pipeline = gst_pipeline_new ("test-pipeline");

  if (!pipeline || !source || !sink || !filter || !converter) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }

  /* Build the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, sink, filter, converter, NULL);
  
  if (gst_element_link_many (source, filter, converter, sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Modify the source's properties */
  g_object_set (source, "pattern", 1, NULL);
  /* Set udpsink ip and port */
  g_object_set (sink, "host", remote_ip.c_str(), NULL);
  g_object_set (sink, "port", static_cast<gint>(port), NULL);

  /* Start playing */
  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Wait until error or EOS */
  bus = gst_element_get_bus (pipeline);
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

  /* Free resources */
  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  return 0;
}

