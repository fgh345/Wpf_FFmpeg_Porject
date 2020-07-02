#include <iostream>
#include "FFmpegReadCameraZ.h"
#include "FFmpegPushRtmp.h"
#include "SDL_LoadImg.h"
#include "ConsoleFFmpegLib.h"
#include "FFMpegAudioPlay.h"
#include "PCMTest.h"
#include "MemoryTest.h"
#include "FFmpegEncodeYUV.h"
#include "FFmpegEncodePCM.h"
#include "MakeUP_DshowToH264.h"
#include "MakeUP_DshowToAAC.h"

int main() {

	//startReadCameraZ();

	//initSDL_LoadImg();

	//cffStart();

	//播放音频
	//ffap_start();

	//转码YUV
	//ffecyuv_start();

	//转码PCM  失败!
	//ffecpcm_start();

	//推送rtmp
	//ffpr_start();

	//PCMTest pc = PCMTest();

	//pc.simplest_pcm16le_to_pcm8((char*)"..\\..\\Test_44.1k_s16le.pcm");
	//pc.simplest_pcm16le_cut_singlechannel((char*)"..\\..\\Test_drum.pcm", 2360, 120);
	//pc.simplest_pcm16le_cut_singlechannel2((char*)"..\\..\\Test_drum.pcm", 2360, 120);

	//short twoB = 4367;
	//short* ptwoB = &twoB;

	//char* oneB = (char*)ptwoB;

	//printf("AA:%d\n", ptwoB);
	//printf("BB:%d\n", oneB[1]);
	//printf("CC:%d\n", *(oneB + 1));
	//printf("DD:%d\n",oneB);

	//mt_start();

	//读取dshow设备编码为h264
	//mp_dth264_start();

	//读取dshow设备编码为aac
	mp_dtaac_start();

	return 0;
}

