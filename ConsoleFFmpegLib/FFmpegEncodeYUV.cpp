#include "FFmpegEncodeYUV.h"

extern "C"
{
//#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libavformat/avformat.h"
};

void ffecyuv_start() {

	AVFormatContext* formatContext_output;
	AVStream* stream_output;
	AVCodecContext* codecContext_output;

	AVCodec* encoder;
	AVPacket* packet;
	uint8_t* yuv_data_buf;
	AVFrame* frame;

	FILE* in_file;
	fopen_s(&in_file,"..\\..\\Test_480x272.yuv", "rb");

	const char* out_file_path = "result_file_encode_yuv.h264";

	avformat_alloc_output_context2(&formatContext_output, NULL, "h264", NULL);


	//Open output URL
	if (avio_open(&formatContext_output->pb, out_file_path, AVIO_FLAG_READ_WRITE) < 0) {
		printf("Failed to open output file! \n");
		return;
	}

	stream_output = avformat_new_stream(formatContext_output, NULL);
	stream_output->time_base.num = 1;
	stream_output->time_base.den = 25;

	codecContext_output = avcodec_alloc_context3(NULL);

	//codecContext->codec_id =AV_CODEC_ID_HEVC;
	codecContext_output->codec_id = formatContext_output->oformat->video_codec;//编码器id
	codecContext_output->codec_type = AVMEDIA_TYPE_VIDEO;//编码器类型
	codecContext_output->pix_fmt = AV_PIX_FMT_YUV420P;//像素格式
	codecContext_output->width = 480;//视频宽
	codecContext_output->height = 272;//视频高
	codecContext_output->time_base.num = 1;//时间基 分子
	codecContext_output->time_base.den = 25;//时间基 分母
	codecContext_output->bit_rate = 400000;//比特率 单位b  400kps
	codecContext_output->gop_size = 250;// 这里二百五 时间基是25一帧 所以每十帧包含一个关键帧(I)
	//H264
	//codecContext->me_range = 16;
	//codecContext->max_qdiff = 4;
	//codecContext->qcompress = 0.6;
	codecContext_output->qmin = 10;
	codecContext_output->qmax = 51;

	//Optional Param
	codecContext_output->max_b_frames = 3;//猜测 这里是十帧中 一个I帧,三个B帧,六个P帧

	avcodec_parameters_from_context(stream_output->codecpar, codecContext_output);

	//Show some Information
	av_dump_format(formatContext_output, 0, out_file_path, 1);

	// Set Option
	AVDictionary* param = 0;
	//H.264
	if (codecContext_output->codec_id == AV_CODEC_ID_H264) {
		av_dict_set(&param, "preset", "slow", 0);
		av_dict_set(&param, "tune", "zerolatency", 0);
	}
	//H.265
	if (codecContext_output->codec_id == AV_CODEC_ID_HEVC) {
		av_dict_set(&param, "preset", "ultrafast", 0);
		av_dict_set(&param, "tune", "zero-latency", 0);
	}

	encoder = avcodec_find_encoder(codecContext_output->codec_id);

	if (avcodec_open2(codecContext_output, encoder, &param) < 0) {
		printf("Failed to open encoder! \n");
		return;
	}

	frame = av_frame_alloc();

	//写入文件头
	avformat_write_header(formatContext_output, NULL);

	packet = av_packet_alloc();
	av_init_packet(packet);


	int elementCount = codecContext_output->width * codecContext_output->height * 3 / 2;// y:u:v=4:1:1 

	int buffer_size = av_image_get_buffer_size(
		codecContext_output->pix_fmt,
		codecContext_output->width,
		codecContext_output->height,
		1);

	yuv_data_buf = new uint8_t[buffer_size];

	int pst_p = 0;

	while (true)
	{
		//Read raw YUV data
		if (fread(yuv_data_buf, 1, elementCount, in_file) <= 0) {
			printf("Failed to read raw data! \n");
			return;
		}
		else if (feof(in_file)) {
			break;
		}

		frame->data[0] = yuv_data_buf;																		// Y
		frame->data[1] = yuv_data_buf + codecContext_output->width * codecContext_output->height;			// U 
		frame->data[2] = yuv_data_buf + codecContext_output->width * codecContext_output->height * 5 / 4;   // V

		//PTS
		frame->pts = pst_p++;
		frame->format = codecContext_output->pix_fmt;
		frame->width = codecContext_output->width;
		frame->height = codecContext_output->height;
		//frame->linesize[0] = codecContext_output->width;
		//frame->linesize[1] = codecContext_output->width;
		//frame->linesize[2] = codecContext_output->width;

		//Encode
		if (avcodec_send_frame(codecContext_output, frame) == 0) {
			if (avcodec_receive_packet(codecContext_output, packet)==0)
			{
				printf("Succeed to encode %d frame!\n", pst_p);
				packet->stream_index = stream_output->index;

				av_write_frame(formatContext_output, packet);

				av_packet_unref(packet);
			}
		}
	
	}
}