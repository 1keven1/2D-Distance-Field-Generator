#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

int maxThreadNum = 1;
int maxTaskQueueSize = 64;

struct TaskData
{
	cv::Point2i self;
	cv::Mat des;
	int spreadFactor;
	int squareSpreadFactor;
};

struct PixelData
{
	cv::Point2i location;
	uchar color;
};

void AddTask(const TaskData& data);
void ThreadRun(const int& id);
void Task(TaskData data, const int& id);
void Clear();

std::queue<TaskData> taskQueue;
std::vector<cv::Mat> mats;
bool exitThread = false;