#pragma once

//url：文件地址
extern "C" _declspec(dllexport) int init_ffmpeg(char* url);

//读取一帧 -1：未取到 1：音频 2：视频 
extern "C" _declspec(dllexport) int read_frame();

//获取音频帧
extern "C" _declspec(dllexport) char* get_audio_frame();

//获取视频帧
extern "C" _declspec(dllexport) char* get_video_frame();

//获取音频缓存大小
extern "C" _declspec(dllexport) int get_audio_buffer_size();

//获取视频缓存大小
extern "C" _declspec(dllexport) int get_video_buffer_size();

//获取视频宽度
extern "C" _declspec(dllexport) int get_video_width();

//获取视频高度
extern "C" _declspec(dllexport) int get_video_height();

//释放资源
extern "C" _declspec(dllexport) void release();