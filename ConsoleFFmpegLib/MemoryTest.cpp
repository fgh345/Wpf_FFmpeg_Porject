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




	const AVCodec* p;
	void* i = 0;
	while ((p = av_codec_iterate(&i))) {
		printf("cccc:%s \n", p->name);
	}
}