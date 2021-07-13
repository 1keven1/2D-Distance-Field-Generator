#include <iostream>
#include <opencv2/opencv.hpp>

constexpr int spreadFactor = 64;

cv::Mat srcImg;
cv::Size srcSize;
cv::Mat desImg;

// 读取图片
cv::Mat ReadImage(const cv::String& fileName)
{
	cv::Mat mat;
	mat = cv::imread(fileName, 1);
	if (mat.empty())
	{
		std::cout << "Image " << fileName << "Read Failed" << std::endl;
		system("Pause");
		exit(0);
	}
	return mat;
}

// 逐像素处理（测试）
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

std::vector<cv::Point2i> GetAllNeighbors(const cv::Point2i parent, const cv::Size matSize, const int spreadFactor)
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

float CalculateDistance(const cv::Point2i parent, const cv::Point2i neighbor)
{
	float distance = sqrt((neighbor - parent).dot(neighbor - parent));
	return distance;
}

void GenerateDistanceField(const cv::Mat src, cv::Mat des, const int spreadFactor)
{
	// TODO: 生成距离场的算法
	// 对每个像素
	for (int i = 0; i < src.rows; i++)
	{
		for (int j = 0; j < src.cols; j++)
		{
			cv::Point self(j, i);
			std::vector<cv::Point2i> neighbors;
			neighbors = GetAllNeighbors(self, srcSize, spreadFactor);

			float nearestOppositeDistance;
			// 这个像素是白
			if (src.at<cv::Vec3b>(self)[0] >= 128)
			{
				nearestOppositeDistance = spreadFactor;
				for (auto neighbor : neighbors)
				{
					if (src.at<cv::Vec3b>(neighbor)[0] < 128)
					{
						float distance = CalculateDistance(self, neighbor);
						if (distance < nearestOppositeDistance)
						{
							nearestOppositeDistance = distance;
						}
					}
				}
			}
			// 这个像素是黑
			else
			{
				nearestOppositeDistance = -spreadFactor;
				for (auto neighbor : neighbors)
				{
					if (src.at<cv::Vec3b>(neighbor)[0] >= 128)
					{
						float distance = CalculateDistance(self, neighbor);
						if (-distance > nearestOppositeDistance)
						{
							nearestOppositeDistance = -distance;
						}
					}
				}
			}
			// 映射到[0, 1]
			float finalResult = (nearestOppositeDistance + spreadFactor) / (2 * spreadFactor);
			// 写入目标图片
			des.at<uchar>(self) = finalResult * 255;
		}
		if (i % 100 == 0)
		{
			std::cout << i << std::endl;
		}
	}
}

int main()
{
	cv::String filePath = "T_Placards_01_D.png";
	srcImg = ReadImage(filePath);
	srcSize = cv::Size(srcImg.cols, srcImg.rows);
	desImg = cv::Mat(srcSize, CV_8UC1, uchar(0));
	
	// PixelProcessing(srcImg, desImg);
	GenerateDistanceField(srcImg, desImg, spreadFactor);

	cv::cvtColor(srcImg, srcImg, cv::COLOR_RGB2BGR);
	cv::resize(srcImg, srcImg, cv::Size(700, 700));
	cv::resize(desImg, desImg, cv::Size(256, 256));

	//cv::imshow("src", srcImg);
	//cv::imshow("des", desImg);
	cv::imwrite("T_Placards_01_DF64_256.png", desImg);
	cv::resize(desImg, desImg, cv::Size(128, 128));
	cv::imwrite("T_Placards_01_DF64_128.png", desImg);

	// cv::waitKey(0);
	return 0;
}