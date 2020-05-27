#include "MemoryTest.h"


extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"  
#include "libswscale/swscale.h"  
#include "libavutil/imgutils.h"  
#include "libavdevice/avdevice.h"
}



void mt_start() {
	avformat_network_init();//支持网络流
	avdevice_register_all();//在使用libavdevice之前，必须先运行avdevice_register_all()对设备进行注册


	while (1)//读取一帧，直到读取到数据
	{
		AVFrame* original_video_frame = av_frame_alloc();//返回一个填充默认值的AVFrame
		av_free(original_video_frame);
	}
}