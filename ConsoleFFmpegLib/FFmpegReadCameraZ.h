#pragma once
#include <string>

//入口
void startReadCameraZ();

// 初始化ffmpeg
int frcz_initFFmpeg();

//打印出所有的驱动名称
void show_dshow_device();

void show_dshow_device_option();

void show_vfw_device();

//初始化SDL窗口 
bool frcz_initSDLWindow();

//关闭SDL窗口
void frcz_closeWindow();

//加载相机
bool loadCamera();

//保存为YUV格式文件
void saveYUVFile();

//保存为YUV420P格式文件
void toSaveYUV420PFile();

//保存为YUV422P格式文件
void toSaveYUV422PFile();

//读取一帧
int read_frame_by_camera();
int read_frame_by_micphone();

//释放资源
void frcz_release();

//开启SDL窗口
int frcz_open_window_fun();

//窗口刷新线程
int frcz_vidio_thread(void* opaque);

//音频解析线程
int frcz_audio_thread(void* opaque);

