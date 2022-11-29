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

typedef struct {
  GstElement *pipeline;
  GstElement *source;
  GstElement *sink;
  GstElement *filter;
  GstElement *deco;
  GstElement *h264enc;
  GstElement *rtp_enc;
} pipeline_t;

static void cb_pad_added_handler (GstElement *src, GstPad *new_pad, pipeline_t *data);

/**
 * @brief main app
 * 
 * @param argc 
 * @param argv 1st parameter is optional remote IP
 * @return int 
 */

int main (int argc, char *argv[])
{

  pipeline_t p;
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
  p.source = gst_element_factory_make ("videotestsrc", "source");
  p.filter = gst_element_factory_make("vertigotv", "filter");
  p.deco = gst_element_factory_make("decodebin", "deco");
  p.h264enc = gst_element_factory_make("x264enc", "enc");
  p.rtp_enc = gst_element_factory_make("rtph264pay", "rtp_enc");
  p.sink = gst_element_factory_make("udpsink", "sink");


  /* Create the empty pipeline */
  p.pipeline = gst_pipeline_new ("test-pipeline");

  if (!p.pipeline || !p.source || !p.filter || !p.deco || !p.h264enc || !p.rtp_enc || !p.sink) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }

  /* Build the pipeline */
  gst_bin_add_many (GST_BIN (p.pipeline), p.source, p.deco, p.h264enc, p.rtp_enc, p.sink, NULL);
  
  if (gst_element_link_many (p.source, p.deco, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (p.pipeline);
    return -1;
  }

  if (gst_element_link_many (p.h264enc, p.rtp_enc, p.sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (p.pipeline);
    return -1;
  }

  /* Modify the source's properties */
  g_object_set (p.source, "pattern", 1, NULL);
  
  /* Set udpsink ip and port */
  g_object_set (p.sink, "host", remote_ip.c_str(), NULL);
  g_object_set (p.sink, "port", static_cast<gint>(port), NULL);

/* Connect to the pad-added signal */
  g_signal_connect (p.deco, "pad-added", G_CALLBACK (cb_pad_added_handler), &p);

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

/* This function will be called by the pad-added signal */
static void cb_pad_added_handler (GstElement *src, GstPad *new_pad, pipeline_t *data) {
  GstPad *sink_pad = NULL;
  GstPad *videosink_pad = gst_element_get_static_pad (data->h264enc, "sink");

  GstPadLinkReturn ret;
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;

  std::cout << "received pad is pad?: " << GST_IS_PAD(new_pad) << std::endl;
  std::cout << "videosink pad is pad?: " << GST_IS_PAD(videosink_pad) << std::endl;

  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

  /* If our converter is already linked, we have nothing to do here */
  if (gst_pad_is_linked (videosink_pad)) {
    g_print ("We are already linked. Ignoring.\n");
    /* Unreference the new pad's caps, if we got them */
    if (new_pad_caps != NULL) gst_caps_unref (new_pad_caps);
    if (videosink_pad != NULL) gst_object_unref (videosink_pad);
    return;
  }

  /* Check the new pad's type */
  new_pad_caps = gst_pad_get_current_caps (new_pad);
  new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
  new_pad_type = gst_structure_get_name (new_pad_struct);
  if (g_str_has_prefix (new_pad_type, "video/x-raw")) {
    sink_pad = videosink_pad;
  }
  else {
    g_print ("It has type '%s' which is not raw video. Ignoring.\n", new_pad_type);
    if (new_pad_caps != NULL) gst_caps_unref (new_pad_caps);
    if (videosink_pad != NULL) gst_object_unref (videosink_pad);
    return;
  }

  std::cout << "new pad name: " << gst_element_get_name(new_pad) << std::endl;
  std::cout << "sink pad name: " << gst_element_get_name(sink_pad) << std::endl;

  /* Attempt the link */
  ret = gst_pad_link (new_pad, sink_pad);
  if (GST_PAD_LINK_FAILED (ret)) {
    g_print ("Type is '%s' but link failed.\n", new_pad_type);
  } else {
    g_print ("Link succeeded (type '%s').\n", new_pad_type);
  }

  if (new_pad_caps != NULL) gst_caps_unref (new_pad_caps);
  if (videosink_pad != NULL) gst_object_unref (videosink_pad);
}