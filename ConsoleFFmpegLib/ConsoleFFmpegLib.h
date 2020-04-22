#pragma once

int cffStart();


//url：文件地址 extern "C" _declspec(dllexport) 
int init_ffmpeg(char* url);

//读取一帧 -1：未取到 1：音频 2：视频 
int read_frame();

//获取音频帧
char* get_audio_frame();

//获取视频帧
char* get_video_frame();

//获取音频缓存大小
int get_audio_buffer_size();

//获取视频缓存大小
int get_video_buffer_size();

//获取视频宽度
int get_video_width();

//获取视频高度
int get_video_height();

//释放资源
void release();
