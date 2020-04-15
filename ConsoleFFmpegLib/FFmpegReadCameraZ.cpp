#include <stdio.h>
#include "FFmpegReadCameraZ.h"


extern "C"
{
	#include "libavcodec/avcodec.h"
	#include "libavformat/avformat.h"
	#include "libswscale/swscale.h"
	#include "libavdevice/avdevice.h"
	#include "SDL.h"
};


void initReadCameraZ() {

	printf("start!\n");

	AVFormatContext* aVFormatContext = avformat_alloc_context();
	int				i, videoindex;
	AVCodecContext* aVCodecContext = avcodec_alloc_context3(NULL);
	AVCodec* aVCodec;

	avformat_network_init();

	//Register Device
	avdevice_register_all();

	showAvfoundationDevice();

	printf("end!");
}


void showAvfoundationDevice() {
	AVFormatContext* aVFormatContext = avformat_alloc_context();

	AVDictionary* options = NULL;

	av_dict_set(&options, "list_devices", "true", 0);

	AVInputFormat* iformat = av_find_input_format("avfoundation");

	printf("==AVFoundation Device Info===\n");

	avformat_open_input(&aVFormatContext, "", iformat, &options);

	printf("===============showAvfoundationDevice==============\n");
}
