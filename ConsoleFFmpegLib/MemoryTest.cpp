#include "MemoryTest.h"


extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"  
#include "libswscale/swscale.h"  
#include "libavutil/imgutils.h"  
#include "libavdevice/avdevice.h"
}



void mt_start() {
	//avformat_network_init();//支持网络流
	avdevice_register_all();//在使用libavdevice之前，必须先运行avdevice_register_all()对设备进行注册

	//void* i = 0;
	//const AVCodec* p;

	////遍历打印编码器
	//while ((p = av_codec_iterate(&i))) {
	//	printf("cccc:%s \n", p->name);
	//}

	void* i = 0;
	const AVInputFormat* fmt = NULL;
	//遍历打印封装格式名
	while ((fmt = av_demuxer_iterate(&i))) {
		printf("cccc:%s \n", fmt->name);
	}
}