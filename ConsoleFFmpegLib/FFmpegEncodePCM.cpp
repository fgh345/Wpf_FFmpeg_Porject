#include "FFmpegEncodePCM.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavformat/avformat.h"
};

void ffecpcm_start() {

	AVFormatContext* formatContext_output;
	AVStream* stream_output;
	AVCodecContext* codecContext_output;

	AVCodec* encoder;
	AVPacket* packet;

	AVFrame* frame;

	FILE* in_file;
	fopen_s(&in_file, "..\\..\\Test_44.1k_s16le.pcm", "rb");

	const char* out_file_path = "result_file_encode_pcm.mp3";

	uint8_t* pcm_data_buf;

	avformat_alloc_output_context2(&formatContext_output, NULL, NULL, out_file_path);

	//Open output URL
	if (avio_open(&formatContext_output->pb, out_file_path, AVIO_FLAG_READ_WRITE) < 0) {
		printf("Failed to open output file! \n");
		return;
	}

	stream_output = avformat_new_stream(formatContext_output, NULL);
	
	codecContext_output = avcodec_alloc_context3(NULL);
	codecContext_output->codec_id = formatContext_output->oformat->audio_codec;//编码器id
	codecContext_output->codec_type = AVMEDIA_TYPE_AUDIO;//编码器类型
	codecContext_output->channel_layout = AV_CH_LAYOUT_STEREO;
	codecContext_output->sample_fmt = AV_SAMPLE_FMT_FLTP;
	codecContext_output->sample_rate = 44100;
	codecContext_output->channels = av_get_channel_layout_nb_channels(codecContext_output->channel_layout);
	codecContext_output->bit_rate = 64000;

	avcodec_parameters_from_context(stream_output->codecpar, codecContext_output);

	//Show some Information
	av_dump_format(formatContext_output, 0, out_file_path, 1);

	encoder = avcodec_find_encoder(codecContext_output->codec_id);

	if (avcodec_open2(codecContext_output, encoder, NULL) < 0) {
		printf("Failed to open encoder! \n");
		return;
	}

	frame = av_frame_alloc();

	frame->nb_samples = codecContext_output->frame_size;
	frame->format = codecContext_output->sample_fmt;

	//写入文件头
	avformat_write_header(formatContext_output, NULL);

	packet = av_packet_alloc();
	av_init_packet(packet);

	pcm_data_buf = new uint8_t[4];

	int size = av_samples_get_buffer_size(NULL, codecContext_output->channels, codecContext_output->frame_size, codecContext_output->sample_fmt, 1);

	
	uint8_t* frame_buf = (uint8_t*)av_malloc(size);

	avcodec_fill_audio_frame(frame, codecContext_output->channels, codecContext_output->sample_fmt, (const uint8_t*)frame_buf, size, 1);


	int pst_p = 0;

	while (true)
	{
		//Read raw YUV data
		if (fread(pcm_data_buf, 1, 4, in_file) <= 0) {
			break;
		}
		else if (feof(in_file)) {
			break;
		}

		frame->data[0] = pcm_data_buf;

		frame->pts = pst_p++;


		//Encode
		if (avcodec_send_frame(codecContext_output, frame) == 0) {
			if (avcodec_receive_packet(codecContext_output, packet) == 0)
			{
				//printf("Succeed to encode %d frame!\n", pst_p);

				//packet->duration = duration;

				printf("pts:%d--dts:%d--duration:%d\n", packet->pts, packet->dts, packet->duration);

				int ret = av_write_frame(formatContext_output, packet);

				if (ret != 0)
					printf("write fail %d!\n", ret);

				av_packet_unref(packet);
			}
		}

	}

	//Write file trailer
	av_write_trailer(formatContext_output);

	//Clean
	if (stream_output) {
		avcodec_close(codecContext_output);
		av_free(frame);
	}
	avio_close(formatContext_output->pb);
	avformat_free_context(formatContext_output);

	fclose(in_file);
}