#include "MakeUP_DshowToAAC.h"
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libavdevice/avdevice.h"
#include "libavutil/time.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
};

#include <iostream>
#include <thread>
using namespace std;

int XError(int errNum)
{
	char buf[1024] = { 0 };
	av_strerror(errNum, buf, sizeof(buf));
	std::cout << buf << std::endl;

	return -1;
}

void mp_dtaac_start() {

	char device_in_url[] = "audio=HKZ (Realtek(R) Audio)";//HKZ (Realtek High Definition Audio(SST)) | ub570 (TC-UB570, Audio Capture) | virtual-audio-capturer | HKZ (Realtek(R) Audio)
	char file_out_path[] = "result_file_microphone_to_aac.aac";
	//char file_out_path[] = "rtmp://49.232.191.221/live/hkz"; //推流地址
	//配置输入

	avdevice_register_all();

	/// <summary>
	/// 输入模块
	/// </summary>

	AVFormatContext* formatContext_input = NULL;

	AVInputFormat* fmt = av_find_input_format("dshow");

	AVDictionary* options = NULL;
	av_dict_set_int(&options, "audio_buffer_size", 20, 0);
	//av_dict_set(&options, "list_devices", "true", 0);

	avformat_open_input(&formatContext_input, device_in_url, fmt, &options);

	av_dict_free(&options);

	avformat_find_stream_info(formatContext_input, NULL);

	AVStream* stream_input = formatContext_input->streams[0];

	//处理解码器
	AVCodec* codec_input = avcodec_find_decoder(stream_input->codecpar->codec_id);

	AVCodecContext* codecContext_input = avcodec_alloc_context3(codec_input);

	avcodec_parameters_to_context(codecContext_input, stream_input->codecpar);
	

	if (avcodec_open2(codecContext_input, codec_input, NULL) != 0)
		printf("开启解码器失败!");

	//打印输入流信息
	av_dump_format(formatContext_input, 0, device_in_url, 0);

	/// <summary>
	/// 输出模块
	/// </summary>


	int out_sample_rate = 44100;
	AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_FLTP;
	uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;

	AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);

	AVCodecContext* codecContext_output = avcodec_alloc_context3(encoder);
	codecContext_output->codec = encoder;
	codecContext_output->sample_fmt = out_sample_fmt;
	codecContext_output->sample_rate = out_sample_rate;
	codecContext_output->channel_layout = out_channel_layout;
	codecContext_output->channels = av_get_channel_layout_nb_channels(out_channel_layout);
	codecContext_output->bit_rate = 128000;
	codecContext_output->codec_tag = 0;//表示不用封装器做编码
	codecContext_output->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	codecContext_output->thread_count = 8;

	if (avcodec_open2(codecContext_output, encoder, NULL) < 0)
		printf("开启编码器失败!");

	AVFormatContext* formatContext_output;//av_guess_format("", ".ac3", NULL)
	avformat_alloc_output_context2(&formatContext_output, NULL, NULL, file_out_path);

	AVStream* stream_output = avformat_new_stream(formatContext_output, encoder);

	avcodec_parameters_from_context(stream_output->codecpar, codecContext_output);

	//Open output URL
	if (avio_open2(&formatContext_output->pb, file_out_path, AVIO_FLAG_READ_WRITE, NULL, NULL) < 0) {
		printf("Failed to open output file! \n");
		return;
	}


	//打印输出流信息
	av_dump_format(formatContext_output, 0, file_out_path, 1);

	///准备解码

	int pst_p = 0;

	AVPacket* avpkt_in = av_packet_alloc();
	AVPacket* avpkt_out = av_packet_alloc();

	AVFrame* frameOriginal = av_frame_alloc();
	AVFrame* frameAAC = av_frame_alloc();

	//写入文件头
	avformat_write_header(formatContext_output, NULL);

	//配置过滤器 重新采用音频
	char args[512];
	const AVFilter* abuffersrc = avfilter_get_by_name("abuffer");
	const AVFilter* abuffersink = avfilter_get_by_name("abuffersink");
	AVFilterInOut* outputs = avfilter_inout_alloc();
	AVFilterInOut* inputs = avfilter_inout_alloc();

	AVFilterContext* buffer_src_ctx;
	AVFilterContext* buffer_sink_ctx;
	AVFilterGraph* filter_graph = avfilter_graph_alloc();
	filter_graph->nb_threads = 1;

	sprintf_s(args, sizeof(args),
		"time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%I64x",
		stream_input->time_base.num,
		stream_input->time_base.den,
		stream_input->codecpar->sample_rate,
		av_get_sample_fmt_name((AVSampleFormat)stream_input->codecpar->format),
		av_get_default_channel_layout(codecContext_input->channels));

	printf("args_in:%s \n", args);

	int ret = avfilter_graph_create_filter(&buffer_src_ctx, abuffersrc, "in",
		args, NULL, filter_graph);
	if (ret != 0)
	{
		XError(ret);
	}

	ret = avfilter_graph_create_filter(&buffer_sink_ctx, abuffersink, "out",
		NULL, NULL, filter_graph);
	if (ret != 0)
	{
		XError(ret);
	}

	static const enum AVSampleFormat out_sample_fmts[] = { AV_SAMPLE_FMT_FLTP, AV_SAMPLE_FMT_NONE };
	static const int64_t out_channel_layouts[] = { stream_output->codecpar->channel_layout, -1 };
	static const int out_sample_rates[] = { stream_output->codecpar->sample_rate , -1 };

	ret = av_opt_set_int_list(buffer_sink_ctx, "sample_fmts", out_sample_fmts, -1,
		AV_OPT_SEARCH_CHILDREN);
	ret = av_opt_set_int_list(buffer_sink_ctx, "channel_layouts", out_channel_layouts, -1,
		AV_OPT_SEARCH_CHILDREN);
	ret = av_opt_set_int_list(buffer_sink_ctx, "sample_rates", out_sample_rates, -1,
		AV_OPT_SEARCH_CHILDREN);

	outputs->name = av_strdup("in");
	outputs->filter_ctx = buffer_src_ctx;;
	outputs->pad_idx = 0;
	outputs->next = NULL;

	inputs->name = av_strdup("out");
	inputs->filter_ctx = buffer_sink_ctx;
	inputs->pad_idx = 0;
	inputs->next = NULL;

	ret = avfilter_graph_parse_ptr(filter_graph, "anull", &inputs, &outputs, nullptr);
	if (ret < 0)
	{
		XError(ret);
	}

	ret = avfilter_graph_config(filter_graph, NULL);
	if (ret != 0)
	{
		XError(ret);
	}

	av_buffersink_set_frame_size(buffer_sink_ctx, 1024);

	for (;;) {
		if (av_read_frame(formatContext_input, avpkt_in) == 0)
		{

			if (0 >= avpkt_in->size)
			{
				continue;
			}

			if (avcodec_send_packet(codecContext_input, avpkt_in) == 0)

				if (avcodec_receive_frame(codecContext_input, frameOriginal) == 0)
				{

					std::cout << "@";

					//pst_p += av_rescale_q(frameOriginal->nb_samples, codecContext_output->time_base, stream_output->time_base);

					//printf("%d ", pst_p);

					//转换数据格式
					ret = av_buffersrc_add_frame_flags(buffer_src_ctx, frameOriginal, AV_BUFFERSRC_FLAG_PUSH);
					if (ret < 0)
					{
						XError(ret);
					}

					ret = av_buffersink_get_frame_flags(buffer_sink_ctx, frameAAC, AV_BUFFERSINK_FLAG_NO_REQUEST);
					if (ret < 0)
					{
						continue;
					}

					//pts 计算
					// second = nb_samples/sample_rate   一帧音频的秒数
					// pts = second/timebase 

					//Encode
					//AVRational itime = stream_input->time_base;
					//AVRational otime = stream_output->time_base;



					frameAAC->pts = pst_p;

					pst_p += av_rescale_q(frameAAC->nb_samples, codecContext_output->time_base, stream_output->time_base);

					if (avcodec_send_frame(codecContext_output, frameAAC) == 0) {
						if (avcodec_receive_packet(codecContext_output, avpkt_out) == 0)
						{
							if (avpkt_out->size > 0) {

								//avpkt_out->pts = av_rescale_q_rnd(avpkt_in->pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));// 前面是原始的 ,后面是要转换成为的
								//avpkt_out->dts = av_rescale_q_rnd(avpkt_in->dts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
								//avpkt_out->duration = av_rescale_q_rnd(avpkt_in->duration, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

								avpkt_out->pts = av_rescale_q(avpkt_out->pts, codecContext_output->time_base, stream_output->time_base);// 前面是原始的 ,后面是要转换成为的
								avpkt_out->dts = av_rescale_q(avpkt_out->dts, codecContext_output->time_base, stream_output->time_base);
								avpkt_out->duration = av_rescale_q(avpkt_out->duration, codecContext_output->time_base, stream_output->time_base);
								avpkt_out->pos = -1;

								//printf("pst_p:%d,pts:%d,dts:%d,size:%d,duration:%d\n", pst_p, avpkt_out->pts, avpkt_out->dts, avpkt_out->size, avpkt_out->duration);

								ret = av_interleaved_write_frame(formatContext_output, avpkt_out);
								if (ret != 0)
								{
									XError(ret);
								}
							}
						}
					}
				}
			av_packet_unref(avpkt_in);

		}
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
}