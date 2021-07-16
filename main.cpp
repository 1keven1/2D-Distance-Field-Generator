#include <iostream>
#include <opencv2/opencv.hpp>
#include <ctime>
#define SQRT2 1.41421356

constexpr int spreadFactor = 64;
constexpr int searchingRingWidth = 10;

cv::Mat srcImg;
cv::Size srcSize;
cv::Mat desImg;


// ��ȡͼƬ
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

// �����ش������ԣ�
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
	int squareRadiusSmall = radiusSmall * radiusSmall;
	// �������ϵ�
	for (int i = radiusSmall + 1; i <= radiusBig; i++)
	{
		if (i + center.x < 0 || i + center.x >= matSize.width) continue;
		points.push_back(cv::Point2i(i, 0));
	}

	// y = x�ϵ�
	int maxXSmall = floor(radiusSmall / SQRT2);
	int maxXBig = floor(radiusBig / SQRT2);
	for (int i = maxXSmall + 1; i <= maxXBig; i++)
	{
		if (i + center.x < 0 || i + center.x >= matSize.width ||i + center.y < 0 || i + center.y >= matSize.height) continue;
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
				//if ((j * j + i * i) < squareRadiusSmall) continue;
			}

			if (j + center.x < 0 || j + center.x >= matSize.width || i + center.y < 0 || i + center.y >= matSize.height) continue;
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
				//if ((j * j + i * i) < squareRadiusSmall) continue;
			}

			if (j + center.x < 0 || j + center.x >= matSize.width || i + center.y < 0 || i + center.y >= matSize.height) continue;
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
			if (points[i].y + center.x < 0 || points[i].y + center.x >= matSize.width || points[i].x + center.y < 0 || points[i].x + center.y >= matSize.height) continue;
			points.push_back(cv::Point2i(points[i].y, points[i].x));
		}
	}

	// ����y��Գ�
	pointNum = points.size();
	for (int i = 0; i < pointNum; i++)
	{
		if (points[i].x != 0)
		{
			if (-points[i].x + center.x < 0 || -points[i].x + center.x >= matSize.width || points[i].y + center.y < 0 || points[i].y + center.y >= matSize.height) continue;
			points.push_back(cv::Point2i(-points[i].x, points[i].y));
		}
	}

	// ����x��Գ�
	pointNum = points.size();
	for (int i = 0; i < pointNum; i++)
	{
		if (points[i].y != 0)
		{
			if (points[i].x + center.x < 0 || points[i].x + center.x >= matSize.width || -points[i].y + center.y < 0 || -points[i].y + center.y >= matSize.height) continue;
			points.push_back(cv::Point2i(points[i].x, -points[i].y));
		}
	}

	// ƽ�Ƶ�Բ��
	for (auto& point : points)
	{
		point.x += center.x;
		point.y += center.y;
	}
	return points;
}

void GenerateDistanceField(const cv::Mat& src, cv::Mat des, const int& spreadFactor)
{
	int squareSpreadFactor = pow(spreadFactor, 2);
	// TODO: ���ɾ��볡���㷨
	// ��ÿ������
	for (int i = 0; i < src.rows; i++)
	{
		for (int j = 0; j < src.cols; j++)
		{
			// �����㷨
			cv::Point self(j, i);
			bool bWhite = src.at<cv::Vec3b>(self)[0] >= 128;
			float nearestOppositeDistance = bWhite ? spreadFactor : -spreadFactor;
			float squreNearestOppositeDistance = squareSpreadFactor;

			// һȦһȦ��
			for (int t = 0; t < spreadFactor; t = t + searchingRingWidth)
			{
				int tBig = ((t + searchingRingWidth) > spreadFactor) ? spreadFactor : t + searchingRingWidth;
				std::vector<cv::Point2i> neighbors;
				neighbors = GetAllPointInRing(self, t, tBig, srcSize);
				bool bFound = false;
				for (auto neighbor : neighbors)
				{
					// �����ɫ�ҵ��˺�����
					if (src.at<cv::Vec3b>(neighbor)[0] < 128 && bWhite)
					{
						bFound = true;
						float squareDistance = CalculateSquareDistance(self, neighbor);
						if (squareDistance < squreNearestOppositeDistance)
						{
							nearestOppositeDistance = sqrt(squareDistance);
						}
					}
					// �����ɫ�ҵ��˰�����
					else if (src.at<cv::Vec3b>(neighbor)[0] >= 128 && !bWhite)
					{
						bFound = true;
						float squareDistance = CalculateSquareDistance(self, neighbor);
						if (squareDistance < squreNearestOppositeDistance)
						{
							nearestOppositeDistance = -sqrt(squareDistance);
						}
					}
				}
				// �ҵ���ֱ������
				if (bFound) break;
			}
			// ӳ�䵽[0, 1]
			float finalResult = (nearestOppositeDistance + spreadFactor) / (2 * spreadFactor);
			// д��Ŀ��ͼƬ
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
	cv::String filePath = "gan1024.png";
	srcImg = ReadImage(filePath);
	srcSize = cv::Size(srcImg.cols, srcImg.rows);
	desImg = cv::Mat(srcSize, CV_8UC1, uchar(0));
	
	clock_t start = clock();
	GenerateDistanceField(srcImg, desImg, spreadFactor);
	clock_t end = clock();
	std::cout << end - start << "ms" << std::endl;

	cv::cvtColor(srcImg, srcImg, cv::COLOR_RGB2BGR);
	cv::resize(srcImg, srcImg, cv::Size(700, 700));
	cv::resize(desImg, desImg, cv::Size(256, 256));

	//cv::imshow("src", srcImg);
	//cv::imshow("des", desImg);
	cv::imwrite("Result_Gan_DF64_256_1.png", desImg);
	//cv::resize(desImg, desImg, cv::Size(128, 128));
	//cv::imwrite("Result_Gan_DF64_128.png", desImg);

	cv::waitKey(0);
	return 0;
}