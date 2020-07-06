#include "AnalysisAAC.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libavdevice/avdevice.h"
#include "libavutil/time.h"
};

void ans_aac_start() {

	char aac_filepath[] = "..\\..\\John Grisham.aac";

	FILE* out_Pcm_File;
	fopen_s(&out_Pcm_File, "result_file_aac.pcm", "wb+");

	AVFormatContext* formatContext_input = avformat_alloc_context();

	AVInputFormat* fmt = av_find_input_format(aac_filepath);

	avformat_open_input(&formatContext_input, aac_filepath, fmt, NULL);

	avformat_find_stream_info(formatContext_input, NULL);

	AVStream* stream_input = formatContext_input->streams[0];

	//打印输入流信息
	av_dump_format(formatContext_input, 0, aac_filepath, 0);

	//处理解码器

	AVCodecContext* codecContext_input = avcodec_alloc_context3(NULL);
	codecContext_input->channel_layout = AV_CH_LAYOUT_STEREO;
	avcodec_parameters_to_context(codecContext_input, stream_input->codecpar);
	AVCodec* codec_input = avcodec_find_decoder(codecContext_input->codec_id);

	if (avcodec_open2(codecContext_input, codec_input, NULL) != 0)
		printf("开启解码器失败!");

	AVPacket* avpkt_in = av_packet_alloc();
	av_init_packet(avpkt_in);

	AVFrame* frameOriginal = av_frame_alloc();

	while (av_read_frame(formatContext_input, avpkt_in) == 0)
	{
		if (avcodec_send_packet(codecContext_input, avpkt_in) == 0)

			if (avcodec_receive_frame(codecContext_input, frameOriginal) == 0)
			{
				printf("pts:%d--nb_samples:%d\n", frameOriginal->pts, frameOriginal->nb_samples);

				int count = frameOriginal->linesize[0] / 2 / 4;

				fwrite(frameOriginal->data[0], 4, count, out_Pcm_File);
				fwrite(frameOriginal->data[1], 4, count, out_Pcm_File);

			}
	}
}