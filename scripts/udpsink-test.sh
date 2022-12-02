print_help() {

    echo "This scripts creates a local udp pipeline to test the local udpsrc C++ program"
    echo -e "\nUsage:"
    echo "   -h print this message"
    echo "   -p port to listen connections"

}

while getopts :hp: flag
do
    case "${flag}" in
        h) print_help
           exit 0;;
        p) PORT=$OPTARG;;
        *) echo "Flag not found"
           print_help
           exit 1;;
    esac
done


gst-launch-1.0 udpsrc port=$PORT ! application/x-rtp, encoding-name=H264, payload=96 ! rtph264depay ! avdec_h264 ! autovideoconvert ! autovideosink