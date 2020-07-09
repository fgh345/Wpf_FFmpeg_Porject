#include "MakeUP_DshowToAAC.h"
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libavdevice/avdevice.h"
#include "libavutil/time.h"
#include "SDL.h"
};

#include <iostream>

int XError(int errNum)
{
	char buf[1024] = { 0 };
	av_strerror(errNum,buf,sizeof(buf));
	std::cout << buf << std::endl;

	return -1;
}

void mp_dtaac_start() {

	char device_in_url[] = "audio=virtual-audio-capturer";//HKZ (Realtek High Definition Audio(SST)) | ub570 (TC-UB570, Audio Capture) | virtual-audio-capturer
	char file_out_path[] = "result_file_microphone_to_aac.aac";
	FILE* out_Pcm_File;
	//char file_out_path[] = "rtmp://192.168.30.20/live/livestream"; //推流地址
	//配置输入

	fopen_s(&out_Pcm_File, "result_file_microphone.pcm", "wb+");

	avdevice_register_all();

	//avformat_network_init();

	//void* i = 0;
	//const AVInputFormat* aa = NULL;
	//遍历打印封装格式名
	//while ((aa = av_demuxer_iterate(&i))) {
	//	printf("cccc:%s \n", aa->name);
	//}

	AVDictionary* options = NULL;
	av_dict_set(&options, "bit_rate", "128000", 0);
	//av_dict_set(&options, "list_devices", "true", 0);
	/*av_dict_set(&options, "list_options", "true", 0);*/


	AVFormatContext* formatContext_input = avformat_alloc_context();

	AVInputFormat* fmt = av_find_input_format("dshow");

	avformat_open_input(&formatContext_input, device_in_url, fmt, &options);

	avformat_find_stream_info(formatContext_input, NULL);

	AVStream* stream_input = formatContext_input->streams[0];
	
	//打印输入流信息
	av_dump_format(formatContext_input, 0, device_in_url, 0);

	//处理解码器

	AVCodecContext* codecContext_input = avcodec_alloc_context3(NULL);
	codecContext_input->channel_layout = AV_CH_LAYOUT_STEREO;
	avcodec_parameters_to_context(codecContext_input, stream_input->codecpar);
	AVCodec* codec_input = avcodec_find_decoder(codecContext_input->codec_id);

	if (avcodec_open2(codecContext_input, codec_input, NULL) != 0)
		printf("开启解码器失败!");

	//配置输出

	int out_sample_rate = 48000;
	AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_FLTP;
	uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;

	AVFormatContext* formatContext_output;//av_guess_format("", ".ac3", NULL)
	avformat_alloc_output_context2(&formatContext_output, NULL, NULL, file_out_path);

	//Open output URL
	if (avio_open(&formatContext_output->pb, file_out_path, AVIO_FLAG_READ_WRITE) < 0) {
		printf("Failed to open output file! \n");
		return;
	}

	AVStream* stream_output = avformat_new_stream(formatContext_output, NULL);
	stream_output->time_base = stream_input->time_base;
	
	AVCodecContext* codecContext_output = avcodec_alloc_context3(NULL);
	codecContext_output->codec_id = formatContext_output->oformat->audio_codec;//编码器id
	codecContext_output->codec_type = AVMEDIA_TYPE_AUDIO;//编码器类型
	codecContext_output->sample_fmt = out_sample_fmt;
	codecContext_output->sample_rate = out_sample_rate;
	codecContext_output->channel_layout = out_channel_layout;
	codecContext_output->channels = av_get_channel_layout_nb_channels(out_channel_layout);
	codecContext_output->bit_rate = 128000;
	codecContext_output->frame_size = 1024;
	codecContext_output->qmax = 31;
	codecContext_output->qmin = 2;
	codecContext_output->max_qdiff = 3;


	avcodec_parameters_from_context(stream_output->codecpar, codecContext_output);

	//打印输出流信息
	av_dump_format(formatContext_output, 0, file_out_path, 1);

	//处理编码器

	AVCodec* encoder = avcodec_find_encoder(codecContext_output->codec_id);
	if (avcodec_open2(codecContext_output, encoder, NULL) < 0)
		printf("开启编码器失败!");

	int pst_p = 0;

	//准备解码
	AVPacket avpkt_in;

	AVPacket* avpkt_out = av_packet_alloc();
	//av_init_packet(avpkt_out);

	AVFrame* frameOriginal = av_frame_alloc();
	AVFrame* frameAAC = av_frame_alloc();
	
	frameAAC->nb_samples = 1024;
	frameAAC->format = out_sample_fmt;
	frameAAC->channel_layout = out_channel_layout;
	frameAAC->channels = av_get_channel_layout_nb_channels(out_channel_layout);
	frameAAC->sample_rate = out_sample_rate;

	av_frame_get_buffer(frameAAC, 1);

	//配置音频编码转换
	SwrContext* swrContext = swr_alloc_set_opts(
		NULL,
		out_channel_layout,                            /*out*/
		out_sample_fmt,                                 /*out*/
		out_sample_rate,                                /*out*/
		av_get_default_channel_layout(codecContext_input->channels),/*in 默认情况 声道布局可能是空的 这里通过声道数 获取一个默认的声道布局*/
		codecContext_input->sample_fmt,                 /*in*/
		codecContext_input->sample_rate,                /*in*/
		0,
		NULL);

	swr_init(swrContext);


	//写入文件头
	avformat_write_header(formatContext_output, NULL);

	//int64_t time_start= av_gettime();

	while (av_read_frame(formatContext_input, &avpkt_in) == 0)
	{
		if (avcodec_send_packet(codecContext_input, &avpkt_in) == 0)

			if (avcodec_receive_frame(codecContext_input, frameOriginal) == 0)
			{

				//frameAAC->nb_samples = frameOriginal->nb_samples;

				//转换数据格式
				int ret = swr_convert(swrContext, frameAAC->data, 1024, (const uint8_t**)frameOriginal->data, frameOriginal->nb_samples);
				XError(ret);

				//int count = frameAAC->linesize[0] / 2 / 4;

				//fwrite(frameAAC->data[0], 4, count, out_Pcm_File);
				//fwrite(frameAAC->data[1], 4, count, out_Pcm_File);
				
				//float* temptr = (float*)frameAAC->data[0];

				//for (int i = 0; i < count; i++)
				//{
				//	printf(" %f ", *(temptr + i));
				//}
				//printf("\n");
				

				//Encode
				frameAAC->pts = 307200 * pst_p;

				printf("frameOriginal->pts:%d\n", frameOriginal->pts);

				if (avcodec_send_frame(codecContext_output, frameAAC) == 0) {
					if (avcodec_receive_packet(codecContext_output, avpkt_out) == 0)
					{

						printf("pts:%d--dts:%d--duration:%d--pst_p:%d\n", avpkt_out->pts, avpkt_out->dts, avpkt_out->duration, pst_p++);

						int ret = av_interleaved_write_frame(formatContext_output, avpkt_out);

						if (ret != 0)
							printf("write fail %d!\n", ret);

						//av_packet_unref(avpkt_out);

					}
				}
			}
		av_packet_unref(&avpkt_in);


		if (pst_p++ > 1000)
			break;

		
	}


	//Write file trailer
	av_write_trailer(formatContext_output);

	//Clean
	if (stream_output) {
		avcodec_close(codecContext_output);
		av_free(frameAAC);
	}
	avio_close(formatContext_output->pb);
	avformat_free_context(formatContext_output);
	swr_free(&swrContext);
}