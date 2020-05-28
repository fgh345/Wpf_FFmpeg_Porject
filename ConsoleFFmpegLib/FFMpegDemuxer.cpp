#include "FFMpegDemuxer.h"

//FLV的文件分离得到H.264视频码流文件和MP3

extern "C"
{
#include "libavformat/avformat.h"
};


void ffmd_start() {

	AVFormatContext* in_AVFormatContext = NULL;
	AVPacket aVPacket;
	int ret, i;
	int videoindex = -1, audioindex = -1;
	const char* in_filename = "tcsn.mp4";//Input file URL
	const char* out_filename_v = "tcsn.h264";//Output file URL
	const char* out_filename_a = "tcsn.aac";

	av_register_all();

	//Input
	if ((ret = avformat_open_input(&in_AVFormatContext, in_filename, 0, 0)) < 0) {
		printf("Could not open input file.");
		return;
	}
	if ((ret = avformat_find_stream_info(in_AVFormatContext, 0)) < 0) {
		printf("Failed to retrieve input stream information");
		return;
	}

	for (i = 0; i < in_AVFormatContext->nb_streams; i++) {
		if (in_AVFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
		}
		else if (in_AVFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioindex = i;
		}
	}

	//Dump Format------------------
	printf("\nInput Video===========================\n");
	av_dump_format(in_AVFormatContext, 0, in_filename, 0);
	printf("\n======================================\n");

	FILE* fp_audio = fopen(out_filename_a, "wb+");
	FILE* fp_video = fopen(out_filename_v, "wb+");

}