#include <iostream>
#include <opencv2/opencv.hpp>
#include <ctime>

constexpr int spreadFactor = 64;

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

float CalculateSquareDistance(const cv::Point2i& parent, const cv::Point2i& neighbor)
{
	float squareDistance = (neighbor - parent).dot(neighbor - parent);
	return squareDistance;
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
			cv::Point self(j, i);
			std::vector<cv::Point2i> neighbors;
			neighbors = GetAllNeighbors(self, srcSize, spreadFactor);

			float nearestOppositeDistance;
			float squreNearestOppositeDistance;
			// ��������ǰ�
			if (src.at<cv::Vec3b>(self)[0] >= 128)
			{
				nearestOppositeDistance = spreadFactor;
				squreNearestOppositeDistance = squareSpreadFactor;
				for (auto neighbor : neighbors)
				{
					if (src.at<cv::Vec3b>(neighbor)[0] < 128)
					{
						float squareDistance = CalculateSquareDistance(self, neighbor);
						if (squareDistance > squareSpreadFactor) continue;
						if (squareDistance < squreNearestOppositeDistance)
						{
							nearestOppositeDistance = sqrt(squareDistance);
							squreNearestOppositeDistance = pow(nearestOppositeDistance, 2);
						}
					}
				}
			}
			// ��������Ǻ�
			else
			{
				nearestOppositeDistance = -spreadFactor;
				squreNearestOppositeDistance = squareSpreadFactor;
				for (auto neighbor : neighbors)
				{
					if (src.at<cv::Vec3b>(neighbor)[0] >= 128)
					{
						float squareDistance = CalculateSquareDistance(self, neighbor);
						if (squareDistance > squareSpreadFactor) continue;
						if (squareDistance < squreNearestOppositeDistance)
						{
							nearestOppositeDistance = -sqrt(squareDistance);
							squreNearestOppositeDistance = pow(nearestOppositeDistance, 2);
						}
					}
				}
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
	cv::imwrite("Result_Gan_DF64_256.png", desImg);
	cv::resize(desImg, desImg, cv::Size(128, 128));
	cv::imwrite("Result_Gan_DF64_128.png", desImg);

	cv::waitKey(0);
	return 0;
}