#pragma once
#include <string>

//入口
void initReadCameraZ();

// 初始化ffmpeg
int initFFmpeg();

//打印出所有的驱动名称
void show_dshow_device();

void show_dshow_device_option();

void show_vfw_device();

//初始化SDL窗口 
bool initSDLWindow();

//关闭SDL窗口
void closeWindow();

//Loads individual image as texture
void loadTexture();

//加载相机
bool loadCamera();

//读取一帧
int read_frame_by_dshow();
