#include <iostream>
#include "Concurrence.h"
#include <ctime>
#include <string>
#include <fstream>

constexpr auto SQRT2 = 1.41421356;
int spreadFactor = 32;
int searchRingWidth = 10;

cv::Mat srcImg;
cv::Size srcSize;
cv::Mat desImg;
std::string saveType;

// ���߳����軥��������������
std::mutex taskLock;
std::mutex desLock;
std::condition_variable taskNotEmpty;
std::condition_variable taskNotFull;

// ���½�����
inline void UpdateProgressBar(float progress)
{
	int barWidth = 70;

	std::cout << "[";
	int pos = barWidth * progress;
	for (int i = 0; i < barWidth; ++i) {
		if (i < pos) std::cout << "=";
		else if (i == pos) std::cout << ">";
		else std::cout << " ";
	}
	std::cout << "] " << int(progress * 100.0) << " %\r";
	std::cout.flush();
};

// ��ȡ�����ļ�
void ReadConfig()
{
	std::ifstream file("config.txt", std::ios::in);
	if (!file)
	{
		std::cout << "WARNING: ��ȡconfig�ļ�ʧ��" << std::endl;
		system("pause");
		exit(0);
	}
	// ���ж�ȡ
	while (1)
	{
		if (file.eof())
		{
			std::cout << "WARNING: config�ļ��޽���ָ��\"End\"" << std::endl;
			system("pause");
			exit(0);
		}
		std::string s;
		file >> s;
		if (s.compare("SpreadFactor") == 0)
		{
			file >> spreadFactor;
		}
		else if (s.compare("SearchRingWidth") == 0)
		{
			file >> searchRingWidth;
		}
		else if (s.compare("SaveType(bmp:0,png:1,jpg:2)") == 0)
		{
			int t;
			file >> t;
			switch (t)
			{
			case 0:
				saveType = ".bmp";
				break;
			case 1:
				saveType = ".png";
				break;
			case 2:
				saveType = ".jpg";
				break;
			default:
				std::cout << "WARNING: SaveType����" << std::endl;
				system("pause");
				exit(0);
				break;
			}
		}
		else if (s.compare("MaxThreadNum") == 0)
		{
			file >> maxThreadNum;
		}
		else if (s.compare("MaxTaskQueueSize") == 0)
		{
			file >> maxTaskQueueSize;
		}
		else if (s.compare("End") == 0)
		{
			std::cout << "��ȡConfig�ļ��ɹ�" << std::endl;
			std::cout << "Max Thread Num: " << maxThreadNum << std::endl;
			std::cout << "Max Task Queue Size: " << maxTaskQueueSize << std::endl;
			std::cout << "Spread Factor: " << spreadFactor << std::endl;
			std::cout << "Search Ring Width: " << searchRingWidth << std::endl;
			std::cout << "============================= Config Read End ============================="<< std::endl;
			break;
		}
		else
		{
			std::cout << "WARNING: config�ļ�����δ֪���ԣ�" << s << std::endl;
			system("pause");
			exit(0);
			break;
		}
	}
}

// ��������ļ�
void CheckConfig()
{
	bool bWarning = false;
	if (spreadFactor <= 0)
	{
		bWarning = true;
		std::cout << "WARNING: SpreadFactorС��0" << std::endl;
	}
	if (searchRingWidth <= 0)
	{
		bWarning = true;
		std::cout << "WARNING: SearchRingWidthС��0" << std::endl;
	}
	if (searchRingWidth > spreadFactor)
	{
		bWarning = true;
		std::cout << "WARNING: SearchRingWidth���ܴ���SpreadFactor" << std::endl;
	}
	if (maxThreadNum <= 0)
	{
		bWarning = true;
		std::cout << "WARNING: MaxThreadNumС��0" << std::endl;
	}
	if (maxTaskQueueSize < maxThreadNum)
	{
		bWarning = true;
		std::cout << "WARNING: MaxTaskQueueSizeС��MaxThreadNum" << std::endl;
	}

	if (bWarning)
	{
		system("pause");
		exit(0);
	}
}

// ����ļ����
std::vector<std::string> CheckFileType(const int argc, char* argv[])
{
	std::vector<std::string> filePaths;
	for (int i = 1; i < argc; i++)
	{
		// ����ļ���ʽ
		std::string filePath = argv[i];
		std::string fileType = filePath.substr(filePath.find_last_of("."), filePath.length());
		std::string fileName = filePath.substr(filePath.find_last_of("\\") + 1, filePath.length());
		// ֧��PNG��JPG��JPEG��BMP
		if (fileType != ".png" && fileType != ".jpg" && fileType != ".jpeg" && fileType != ".PNG" && fileType != ".JPG" && fileType != ".JPEG" && fileType != ".bmp" && fileType != ".BMP")
		{
			// ���Ծ�����
			std::cout << "WARNING���ļ�" << fileName << "��ʽ����" << std::endl << std::endl;
			continue;
		}
		filePaths.push_back(filePath);
	}
	std::cout << "============================= �ļ���ʽ������ =============================" << std::endl;
	return filePaths;
}

// ��ȡͼƬ
cv::Mat ReadImage(const cv::String& fileName)
{
	cv::Mat mat;
	mat = cv::imread(fileName, 1);
	if (mat.empty())
	{
		std::cout << "WARNING: �ļ� " << fileName << "��ȡʧ��" << std::endl;
	}
	return mat;
}

// DEPRECATED: �����ش������ԣ�
void PixelProcessing(const cv::Mat src, cv::Mat des)
{
	for (int i = 0; i < src.rows; i++)
	{
		for (int j = 0; j < src.cols; j++)
		{
			if (i < 20)
			{
				des.at<uchar>(i, j) = 255;
			}
		}
	}
}

// DEPRECATED: ��ȡ�������������ھ�
std::vector<cv::Point2i> GetAllNeighbors(const cv::Point2i parent, const cv::Size& matSize, const int& spreadFactor)
{
	std::vector<cv::Point2i> neighbors;
	int minX, maxX, minY, maxY;
	minX = MAX(parent.x - spreadFactor, 0);
	maxX = MIN(parent.x + spreadFactor, matSize.width);
	minY = MAX(parent.y - spreadFactor, 0);
	maxY = MIN(parent.y + spreadFactor, matSize.height);

	for (int i = minY; i < maxY; i++)
	{
		for (int j = minX; j < maxX; j++)
		{
			cv::Point2i neighbor(j, i);
			if (neighbor == parent) continue;
			neighbors.push_back(neighbor);
		}
	}
	return neighbors;
}

// ����ƽ������
float CalculateSquareDistance(const cv::Point2i& parent, const cv::Point2i& neighbor)
{
	float squareDistance = (neighbor - parent).dot(neighbor - parent);
	return squareDistance;
}

// ��ȡԲ����������
std::vector<cv::Point2i> GetAllPointInRing(const cv::Point2i& center, const int& radiusSmall, const int& radiusBig, const cv::Size& matSize)
{
	std::vector<cv::Point2i> points;
	// �������ϵ�
	for (int i = radiusSmall + 1; i <= radiusBig; i++)
	{
		points.push_back(cv::Point2i(i, 0));
	}

	// y = x�ϵ�
	int maxXSmall = floor(radiusSmall / SQRT2);
	int maxXBig = floor(radiusBig / SQRT2);
	for (int i = maxXSmall + 1; i <= maxXBig; i++)
	{
		points.push_back(cv::Point2i(i, i));
	}

	// ������
	// �²���
	int maxXBottomSmall = radiusSmall - 1;
	int maxYBottomSmall = floor(sqrt(radiusSmall * radiusSmall - maxXBottomSmall * maxXBottomSmall));
	int maxXBottomBig = radiusBig - 1;
	int maxYBottomBig = floor(sqrt(radiusBig * radiusBig - maxXBottomBig * maxXBottomBig));

	for (int i = 1; i <= maxYBottomBig; i++)
	{
		// �������СԲ�ϰ벿�֣�ÿ���ȼ�����һ��СԲ�����xֵ
		// ֻҪ��СԲ�Խ���yֵ��ĵ�һ������СԲ��������ﲻ����
		int xMaxUpperSmall = 0;
		if (i > maxYBottomSmall && i <= maxXSmall)
		{
			xMaxUpperSmall = floor(sqrt(radiusSmall * radiusSmall - i * i));
		}
		for (int j = i + 1; j <= maxXBottomBig; j++)
		{
			// �����СԲ��ֱ��������ֱ�Ӷ���
			if (j <= maxXBottomSmall && i <= maxYBottomSmall) continue;
			// �����СԲ�ڣ�Ҳ����
			if (i > maxYBottomSmall && i <= maxXSmall)
			{
				if (j <= xMaxUpperSmall) continue;
			}
			points.push_back(cv::Point2i(j, i));
		}
	}

	// �ϲ���
	for (int i = maxYBottomBig + 1; i <= maxXBig; i++)
	{
		int xMaxUpper = floor(sqrt(radiusBig * radiusBig - i * i));
		// ����������һ����ֻҪ��СԲ�Խ���yֵ�󣬾�һ������СԲ�����Ҫ�ж�
		int xMaxUpperSmall = 0;
		if (i <= maxXSmall)
		{
			xMaxUpperSmall = floor(sqrt(radiusSmall * radiusSmall - i * i));
		}
		for (int j = i + 1; j <= xMaxUpper; j++)
		{
			if (i <= maxXSmall)
			{
				if (j <= xMaxUpperSmall) continue;
			}
			points.push_back(cv::Point2i(j, i));
		}
	}

	// �Գ�
	// ����y = x�Գ�
	int pointNum;
	pointNum = points.size();
	for (int i = 0; i < pointNum; i++)
	{
		if (points[i].x != points[i].y)
		{
			points.push_back(cv::Point2i(points[i].y, points[i].x));
		}
	}

	// ����y��Գ�
	pointNum = points.size();
	for (int i = 0; i < pointNum; i++)
	{
		if (points[i].x != 0)
		{
			points.push_back(cv::Point2i(-points[i].x, points[i].y));
		}
	}

	// ����x��Գ�
	pointNum = points.size();
	for (int i = 0; i < pointNum; i++)
	{
		if (points[i].y != 0)
		{
			points.push_back(cv::Point2i(points[i].x, -points[i].y));
		}
	}


	// ƽ�Ƶ�Բ�ģ�������ڷ�Χ��������
	std::vector<cv::Point2i> newPoints;
	for (auto& point : points)
	{
		point.x += center.x;
		point.y += center.y;
		if (point.x < 0 || point.x >= matSize.width || point.y < 0 || point.y >= matSize.height) continue;
		newPoints.push_back(point);
	}

	return newPoints;
}

// ���ɾ��볡�㷨
void GenerateDistanceField(const cv::Mat& src, cv::Mat des, const int& spreadFactor)
{
	// �����̲߳���������
	exitThread = false;
	std::vector<std::thread*> threads;
	for (int i = 0; i < maxThreadNum; i++)
	{
		cv::Mat mat;
		src.copyTo(mat);
		mats.push_back(mat);
		std::thread* t = new std::thread([=] {ThreadRun(i); });
		threads.push_back(t);
	}

	int squareSpreadFactor = pow(spreadFactor, 2);
	// ��ÿ������
	for (int i = 0; i < srcSize.height; i++)
	{
		for (int j = 0; j < srcSize.width; j++)
		{
			cv::Point2i self(j, i);
			// ����������������
			TaskData task = { self, des, spreadFactor, squareSpreadFactor };
			AddTask(task);
		}
		if (i % 10 == 0)
		{
			UpdateProgressBar(static_cast<float>(i) / srcSize.height);
		}
	}
	UpdateProgressBar(1);

	exitThread = true;
	// ����Ϊ�˷�ֹ�е��̻߳��ڵȴ�taskNotEmpty�������߳�����
	taskNotEmpty.notify_all();
	// �ϲ��߳�
	for (const auto& t : threads)
	{
		t->join();
		delete t;
	}
}

// �������
void AddTask(const TaskData& data)
{
	while (1)
	{
		std::unique_lock l(taskLock);
		// �����������п�λ
		if (taskQueue.size() < maxTaskQueueSize)
		{
			taskQueue.push(data);
			l.unlock();
			taskNotEmpty.notify_all();
			break;
		}
		// �ȴ���������п�λ
		else
		{
			taskNotFull.wait(l);
		}
	}
}

// �߳�����
void ThreadRun(const int& id)
{
	while (1)
	{
		std::unique_lock l(taskLock);
		//����������������
		if (!taskQueue.empty())
		{
			TaskData data = taskQueue.front();
			taskQueue.pop();
			l.unlock();
			// ���м�������
			Task(data, id);
			taskNotFull.notify_all();
		}
		// ����������������ѭ��
		else if (exitThread)
		{
			break;
		}
		// �ȴ�������м�������
		else
		{
			taskNotEmpty.wait(l);
		}
	}
}

// ��������
void Task(TaskData data, const int& id)
{
	bool bWhite = mats[id].at<cv::Vec3b>(data.self)[0] >= 128;

	float nearestOppositeDistance = bWhite ? spreadFactor : -spreadFactor;
	float squreNearestOppositeDistance = data.squareSpreadFactor;

	// һȦһȦ����Ѱ��
	for (int t = 0; t < spreadFactor; t = t + searchRingWidth)
	{
		int tBig = ((t + searchRingWidth) > spreadFactor) ? spreadFactor : t + searchRingWidth;
		std::vector<cv::Point2i> neighbors;
		neighbors = GetAllPointInRing(data.self, t, tBig, srcSize);
		bool bFound = false;

		std::vector<PixelData> neighborData;

		for (const auto& neighbor : neighbors)
		{
			PixelData pd = { neighbor, mats[id].at<cv::Vec3b>(neighbor)[0] };
			neighborData.push_back(pd);
		}

		for (const auto& neighbor : neighborData)
		{
			// �����ɫ�ҵ��˺�����
			if (neighbor.color < 128 && bWhite)
			{
				bFound = true;
				float squareDistance = CalculateSquareDistance(data.self, neighbor.location);
				if (squareDistance < squreNearestOppositeDistance)
				{
					nearestOppositeDistance = sqrt(squareDistance);
					squreNearestOppositeDistance = squareDistance;
				}
			}
			// �����ɫ�ҵ��˰�����
			else if (neighbor.color >= 128 && !bWhite)
			{
				bFound = true;
				float squareDistance = CalculateSquareDistance(data.self, neighbor.location);
				if (squareDistance < squreNearestOppositeDistance)
				{
					nearestOppositeDistance = -sqrt(squareDistance);
					squreNearestOppositeDistance = squareDistance;
				}
			}
		}
		// �ҵ���ֱ������
		if (bFound) break;
	}
	// ӳ�䵽[0, 1]
	float finalResult = (nearestOppositeDistance + spreadFactor) / (2 * spreadFactor);

	// д��Ŀ��ͼƬ
	std::unique_lock dl(desLock);
	data.des.at<uchar>(data.self) = finalResult * 255;
	dl.unlock();
}

void Clear()
{
	std::queue<TaskData> empty;
	std::swap(empty, taskQueue);
	mats.clear();
}

int main(int argc, char* argv[])
{
	// ��ȡ�����ļ�
	ReadConfig();
	CheckConfig();

	// ���������ļ�����
	std::vector<std::string> filePaths = CheckFileType(argc, argv);
	if (filePaths.size() <= 0)
	{
		std::cout << "û�д�������ļ�" << std::endl << std::endl;
		system("pause");
		exit(0);
	}

	std::cout << "����" << filePaths.size() << "���ļ���Ҫ����" << std::endl << std::endl;
	std::cout << "----------------------------- START -----------------------------" << std::endl;

	int fileNum = 0;
	clock_t start = clock();
	for (const auto& filePath : filePaths)
	{
		fileNum++;

		// ��ʼ�����ļ�
		std::string fileName = filePath.substr(filePath.find_last_of("\\") + 1, filePath.length());
		std::cout << "���ڴ����" << fileNum << "���ļ���" << fileName << std::endl;

		srcImg = ReadImage(filePath);
		if (srcImg.empty()) continue;

		srcSize = cv::Size(srcImg.cols, srcImg.rows);
		desImg = cv::Mat(srcSize, CV_8UC1, uchar(0));

		clock_t start = clock();
		GenerateDistanceField(srcImg, desImg, spreadFactor);
		clock_t end = clock();
		
		// �����ļ�
		int n = filePath.find_last_of(".");
		std::string DesName = filePath.substr(0, n) + "_DF" + saveType;
		cv::imwrite(DesName, desImg);
		std::cout << std::endl << "������ɣ���ʱ" << static_cast<float>(end - start) / 1000 << "s" << std::endl << std::endl;

		// �������
		Clear();
	}
	clock_t end = clock();

	std::cout << "------------------------------ END ------------------------------" << std::endl << std::endl;
	std::cout << "������ɣ����ι�����" << filePaths.size() << "���ļ�" << std::endl;
	std::cout << "����ʱ" << static_cast<float>(end - start) / 1000 << "s" << std::endl << std::endl;;
	std::cout << "������ɵ��ļ�·����" << std::endl;

	// ��ӡ�������·��
	for (const auto& filePath : filePaths)
	{
		int n = filePath.find_last_of(".");
		std::cout << filePath.substr(0, n) + "_DF" + saveType << std::endl;
	}

	std::cout << std::endl;
	system("pause");
	return 0;
}