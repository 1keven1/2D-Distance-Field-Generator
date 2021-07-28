#include <iostream>
#include <opencv2/opencv.hpp>
#include <ctime>
#include <string>

constexpr auto SQRT2 = 1.41421356;
constexpr int spreadFactor = 32;
constexpr int searchingRingWidth = 10;

cv::Mat srcImg;
cv::Size srcSize;
cv::Mat desImg;

inline void UpdateProgress(float progress)
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

float CalculateSquareDistance(const cv::Point2i& parent, const cv::Point2i& neighbor)
{
	float squareDistance = (neighbor - parent).dot(neighbor - parent);
	return squareDistance;
}

std::vector<cv::Point2i> GetAllPointInRing(const cv::Point2i& center, const int& radiusSmall, const int& radiusBig, const cv::Size& matSize)
{
	assert(radiusBig > radiusSmall);
	assert(radiusSmall >= 0);

	std::vector<cv::Point2i> points;
	// 坐标轴上的
	for (int i = radiusSmall + 1; i <= radiusBig; i++)
	{
		points.push_back(cv::Point2i(i, 0));
	}

	// y = x上的
	int maxXSmall = floor(radiusSmall / SQRT2);
	int maxXBig = floor(radiusBig / SQRT2);
	for (int i = maxXSmall + 1; i <= maxXBig; i++)
	{
		points.push_back(cv::Point2i(i, i));
	}

	// 其他的
	// 下部分
	int maxXBottomSmall = radiusSmall - 1;
	int maxYBottomSmall = floor(sqrt(radiusSmall * radiusSmall - maxXBottomSmall * maxXBottomSmall));
	int maxXBottomBig = radiusBig - 1;
	int maxYBottomBig = floor(sqrt(radiusBig * radiusBig - maxXBottomBig * maxXBottomBig));

	for (int i = 1; i <= maxYBottomBig; i++)
	{
		// 如果到了小圆上半部分，每行先计算那一行小圆的最大x值
		// 只要比小圆对角线y值大的点一定不在小圆里，所以这里不计算
		int xMaxUpperSmall = 0;
		if (i > maxYBottomSmall && i <= maxXSmall)
		{
			xMaxUpperSmall = floor(sqrt(radiusSmall * radiusSmall - i * i));
		}
		for (int j = i + 1; j <= maxXBottomBig; j++)
		{
			// 如果在小圆的直角梯形内直接丢弃
			if (j <= maxXBottomSmall && i <= maxYBottomSmall) continue;
			// 如果在小圆内，也丢弃
			if (i > maxYBottomSmall && i <= maxXSmall)
			{
				if (j <= xMaxUpperSmall) continue;
			}
			points.push_back(cv::Point2i(j, i));
		}
	}

	// 上部分
	for (int i = maxYBottomBig + 1; i <= maxXBig; i++)
	{
		int xMaxUpper = floor(sqrt(radiusBig * radiusBig - i * i));
		// 这里与上面一样，只要比小圆对角线y值大，就一定不在小圆里，不需要判断
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

	// 对称
	// 根据y = x对称
	int pointNum;
	pointNum = points.size();
	for (int i = 0; i < pointNum; i++)
	{
		if (points[i].x != points[i].y)
		{
			points.push_back(cv::Point2i(points[i].y, points[i].x));
		}
	}

	// 根据y轴对称
	pointNum = points.size();
	for (int i = 0; i < pointNum; i++)
	{
		if (points[i].x != 0)
		{
			points.push_back(cv::Point2i(-points[i].x, points[i].y));
		}
	}

	// 根据x轴对称
	pointNum = points.size();
	for (int i = 0; i < pointNum; i++)
	{
		if (points[i].y != 0)
		{
			points.push_back(cv::Point2i(points[i].x, -points[i].y));
		}
	}


	// 平移到圆心，如果不在范围内则抛弃
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

void GenerateDistanceField(const cv::Mat& src, cv::Mat des, const int& spreadFactor)
{
	int squareSpreadFactor = pow(spreadFactor, 2);
	// 对每个像素
	for (int i = 0; i < srcSize.height; i++)
	{
		for (int j = 0; j < srcSize.width; j++)
		{
			// 船新算法
			cv::Point self(j, i);
			bool bWhite = src.at<cv::Vec3b>(self)[0] >= 128;
			float nearestOppositeDistance = bWhite ? spreadFactor : -spreadFactor;
			float squreNearestOppositeDistance = squareSpreadFactor;

			// 一圈一圈找
			for (int t = 0; t < spreadFactor; t = t + searchingRingWidth)
			{
				int tBig = ((t + searchingRingWidth) > spreadFactor) ? spreadFactor : t + searchingRingWidth;
				std::vector<cv::Point2i> neighbors;
				neighbors = GetAllPointInRing(self, t, tBig, srcSize);
				bool bFound = false;
				for (const auto& neighbor : neighbors)
				{
					// 如果白色找到了黑像素
					if (src.at<cv::Vec3b>(neighbor)[0] < 128 && bWhite)
					{
						bFound = true;
						float squareDistance = CalculateSquareDistance(self, neighbor);
						if (squareDistance < squreNearestOppositeDistance)
						{
							nearestOppositeDistance = sqrt(squareDistance);
							squreNearestOppositeDistance = squareDistance;
						}
					}
					// 如果黑色找到了白像素
					else if (src.at<cv::Vec3b>(neighbor)[0] >= 128 && !bWhite)
					{
						bFound = true;
						float squareDistance = CalculateSquareDistance(self, neighbor);
						if (squareDistance < squreNearestOppositeDistance)
						{
							nearestOppositeDistance = -sqrt(squareDistance);
							squreNearestOppositeDistance = squareDistance;
						}
					}
				}
				// 找到了直接跳出
				if (bFound) break;
			}
			// 映射到[0, 1]
			float finalResult = (nearestOppositeDistance + spreadFactor) / (2 * spreadFactor);
			// 写入目标图片
			des.at<uchar>(self) = finalResult * 255;
		}
		if (i % 10 == 0)
		{
			UpdateProgress(static_cast<float>(i) / srcSize.height);
		}
	}
	UpdateProgress(1);
}


int main(int argc, char* argv[])
{
	std::cout << "共有" << argc - 1 << "个文件需要处理" << std::endl;
	std::cout << "-----------------------------START-----------------------------" << std::endl;
	for (int i = 1; i < argc; i++)
	{
		// 检查文件格式
		std::string filePath = argv[i];
		std::string fileType = filePath.substr(filePath.find_last_of("."), filePath.length());
		std::string fileName = filePath.substr(filePath.find_last_of("\\") + 1, filePath.length());
		if (fileType != ".png" && fileType != ".jpg" && fileType != ".jpeg" && fileType != ".PNG" && fileType != ".JPG" && fileType != ".JPEG")
		{
			std::cout << "文件" << fileName << "格式有误" << std::endl;
			continue;
		}

		std::cout << "正在处理第" << i << "个文件：" << fileName << std::endl;
		srcImg = ReadImage(filePath);
		srcSize = cv::Size(srcImg.cols, srcImg.rows);
		desImg = cv::Mat(srcSize, CV_8UC1, uchar(0));

		clock_t start = clock();
		GenerateDistanceField(srcImg, desImg, spreadFactor);
		clock_t end = clock();

		std::cout << std::endl << "耗时" << static_cast<float>(end - start) / 1000 << "s" << std::endl << std::endl;

		// 储存文件
		int n = filePath.find_last_of(".");
		std::string DesName = filePath.substr(0, n) + "_DF.png";
		cv::imwrite(DesName, desImg);
	}

	system("pause");
	return 0;
}