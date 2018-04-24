#include "AlgorithmEstimation.hpp"
#include <fstream>
#include <iterator>
#include <cstdint>

bool computeMatchesDistanceStatistics(const Matches& matches, float& meanDistance, float& stdDev)
{
    if (matches.empty())
        return false;
    
    std::vector<float> distances(matches.size());
    for (size_t i=0; i<matches.size(); i++)
        distances[i] = matches[i].distance;
    
    cv::Scalar mean, dev;
    cv::meanStdDev(distances, mean, dev);
    
    meanDistance = static_cast<float>(mean.val[0]);
    stdDev       = static_cast<float>(dev.val[0]);
    
    return false;
}

float distance(const cv::Point2f a, const cv::Point2f b)
{
    return sqrt((a - b).dot(a-b));
}

cv::Scalar computeReprojectionError(const Keypoints& source, const Keypoints& query, const Matches& matches, const cv::Mat& homography);


bool performEstimation
(
 const FeatureAlgorithm& alg,//��һ������ �����㷨����
 const ImageTransformation& transformation,//�ڶ������� ����任����
 const cv::Mat& sourceImage,
 std::vector<FrameMatchingStatistics>& stat
)
{
    Keypoints   sourceKp;
    Descriptors sourceDesc;

    cv::Mat gray;

    if (sourceImage.channels() == 3)
    {
        cv::cvtColor(sourceImage, gray, cv::COLOR_BGR2GRAY);
    }
    else if (sourceImage.channels() == 4)
    {
        cv::cvtColor(sourceImage, gray, cv::COLOR_BGRA2GRAY);
    }
    else if(sourceImage.channels() == 1)
    {
        gray = sourceImage;
    }

    if (!alg.extractFeatures(gray, sourceKp, sourceDesc))//��ȡԭͼ�ĻҶ�ͼ���������������
        return false;
    
    std::vector<float> x = transformation.getX();//�任�Ĳ�����ɵ�����
    stat.resize(x.size());
    
    const int count = x.size();
    
    Keypoints   resKpReal;
    Descriptors resDesc;
    Matches     matches,inliermatches;
    
    // To convert ticks to milliseconds
    const double toMsMul = 1000. / cv::getTickFrequency();//����Ϊ��λ
    
    //#pragma omp parallel for private(resKpReal, resDesc, matches) schedule(dynamic, 5)
    for (int i = 0; i < count; i++)
    {
        float       arg = x[i];
        FrameMatchingStatistics& s = stat[i];//s���������׼Ч���ĸ��ֲ���
        
        cv::Mat     transformedImage;
        transformation.transform(arg, gray, transformedImage);//ͨ�����صõ���ͬ�任���ͼ��

        if (0)
        {
            std::ostringstream image_name;
            image_name << "image_dump_" << transformation.name << "_" << i << ".bin";
            std::ofstream dump(image_name.str().c_str(), std::ios::binary);
            std::copy(transformedImage.datastart, transformedImage.dataend, std::ostream_iterator<uint8_t>(dump));
        }
        cv::Mat expectedHomography = transformation.getHomography(arg, gray);//????ֱ�ӿ�����ֻ����һ����λ�󣬵���ֻ�ǳ�ʼ��Ҳ������
                
        int64 start = cv::getTickCount();
        
        alg.extractFeatures(transformedImage, resKpReal, resDesc);//��ȡ�任��ͼ����������������

        // Initialize required fields
        s.isValid        = resKpReal.size() > 0;
        s.argumentValue  = arg;
        
        if (!s.isValid)
            continue;

        alg.matchFeatures(sourceDesc, resDesc, matches);//������ͼ��������train��query�õ�ƥ���
		//���������knn�Ż� ������ֱ����rob Hess�Ĵ��룬���Ĵ�����c����д��

        int64 end = cv::getTickCount();

        std::vector<cv::Point2f> sourcePoints, sourcePointsInFrame;
        cv::KeyPoint::convert(sourceKp, sourcePoints);// sourceKp��Keypoints���͵� ת��ΪPoint2f
        cv::perspectiveTransform(sourcePoints, sourcePointsInFrame, expectedHomography);//��ԭͼ�������㼯ͨ������任������һ�飬��Ӧ����任���
		//you want to compute the most probable perspective transformation out of several
			/*pairs of corresponding points, you can use getPerspectiveTransform or
			findHomography .*/
        cv::Mat homography;

        //so, we have :
        //N - number of keypoints in the first image that are also visible
        //    (after transformation) on the second image

        //    N1 - number of keypoints in the first image that have been matched.

        //    n - number of the correct matches found by the matcher

        //    n / N1 - precision
        //    n / N - recall(? )

        int visibleFeatures = 0;
        int correctMatches  = 0;
        int matchesCount    = matches.size();

        for (int i = 0; i < sourcePoints.size(); i++)
        {
            if (sourcePointsInFrame[i].x > 0 &&
                sourcePointsInFrame[i].y > 0 &&
                sourcePointsInFrame[i].x < transformedImage.cols &&
                sourcePointsInFrame[i].y < transformedImage.rows)
            {
                visibleFeatures++;//ԭͼ��������������任���������ͼ�������ڵĵ���
            }
        }

        for (int i = 0; i < matches.size(); i++)
        {
            cv::Point2f expected = sourcePointsInFrame[matches[i].trainIdx];////trainIdxΪtrain�����ӵ�������match�����к�����Ǹ������� 
            cv::Point2f actual   = resKpReal[matches[i].queryIdx].pt;// //queryIdxΪquery�����ӵ�������match������ǰ����Ǹ�������  //����о�Ӧ��Ҳ��trainIdx������������Ҳ�Ƕ�����
            
            if (distance(expected, actual) < 3.0)
            {
                correctMatches++;//Ԥ�ڵ�ͱ任��ͼ��ƥ�������������С��3����Ϊ����ȷƥ��
            }
        }

       // bool homographyFound = ImageTransformation::findHomography(sourceKp, resKpReal, matches, correctMatches, homography);
		bool homographyFound = ImageTransformation::findHomography(sourceKp, resKpReal, matches, inliermatches, homography);//���ﱾ����ע�͵��ģ�ֱ���ûᱨ��  ������ǵ�һ�������任���ڶ����ı任����
		//��ѡ��least-median����RANSAC������ڵ�Ժ͹��Ƴ��ı任���󣬷���һ��boolֵ
		//inlinermatches��Ϊ�˼���任������ڵ㣬��ղ��жϵ��Ƿ�����ȷƥ��Բ�ǣ��

        // Some simple stat:
        s.isValid        = homographyFound;
        s.totalKeypoints = resKpReal.size();
        s.consumedTimeMs = (end - start) * toMsMul;//ƥ���ʱ
        s.precision = correctMatches / (float) matchesCount;
        s.recall = correctMatches / (float) visibleFeatures;//��ȷƥ�����ռ�ɼ�������ı���

		s.correctMatchesPercent = s.precision;
		s.percentOfMatches = (float)matchesCount / (s.totalKeypoints);
		//matchingRatio��ͨ�����϶��ߵĳ˻����� Ҫ��Ȼmatch����0��matchingRatio()     const { return correctMatchesPercent * percentOfMatches * 100.0f; };

		
        
        // Compute matching statistics
        //if (homographyFound)
        //{
        //    cv::Mat r = expectedHomography * homography.inv();
        //    float error = cv::norm(cv::Mat::eye(3,3, CV_64FC1) - r, cv::NORM_INF);

        //    computeMatchesDistanceStatistics(correctMatches, s.meanDistance, s.stdDevDistance);
        //    s.reprojectionError = computeReprojectionError(sourceKp, resKpReal, correctMatches, homography);
        //    s.homographyError = std::min(error, 1.0f);

        //    if (0 && error >= 1)
        //    {
        //        std::cout << "H expected:" << expectedHomography << std::endl;
        //        std::cout << "H actual:"   << homography << std::endl;
        //        std::cout << "H error:"    << error << std::endl;
        //        std::cout << "R error:"    << s.reprojectionError(0) << ";" 
        //                                   << s.reprojectionError(1) << ";" 
        //                                   << s.reprojectionError(2) << ";" 
        //                                   << s.reprojectionError(3) << std::endl;
        //        
        //        cv::Mat matchesImg;
        //        cv::drawMatches(transformedImage,
        //                        resKpReal,
        //                        gray,
        //                        sourceKp,
        //                        correctMatches,
        //                        matchesImg,
        //                        cv::Scalar::all(-1),
        //                        cv::Scalar::all(-1),
        //                        std::vector<char>(),
        //                        cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
        //        
        //        cv::imshow("Matches", matchesImg);
        //        cv::waitKey(-1);
        //    }
        //}

		// Compute matching statistics �ⲿ��һ��ʼҲ��ע�͵���
		if (homographyFound)
		{
			cv::Mat r = expectedHomography * homography.inv();//��������,����������õõ�һ����λ��
			float error = cv::norm(cv::Mat::eye(3, 3, CV_64FC1) - r, cv::NORM_INF);//��������� �õ������������Ϊ��λ norm������ ����Ԫ�ؾ���ֵ��ƽ�����ٿ�ƽ��
			// ֪���� ʹ��opencv��ν������������㼯֮������ƶȱȽϵģ���һ���𰸾����÷���
			//�������ܺý��д�С�Ƚϣ�������������ô�Ƚϣ����������˷���

			computeMatchesDistanceStatistics(inliermatches, s.meanDistance, s.stdDevDistance);//������Զ�֮��ľ����ֵ�ͱ�׼�� ���������ƥ�����RANSAC֮ǰ����֮���أ�
			s.reprojectionError = computeReprojectionError(sourceKp, resKpReal, matches, homography);//�õ���ԶԵľ����ֵ����׼�� ���ֵ ��Сֵ��ɵ�scalar
			s.homographyError = std::min(error, 1.0f);//ֱ��1.0��double�ͣ������f���float�͵�
			//ֻ���С��1�Ĵ���

			//����psnr
			cv::Mat srcImage = transformedImage.clone();
			cv::Mat dstImage = sourceImage.clone();
			//cv::perspectiveTransform(srcImage, dstImage,homography.inv()); //������׼ͼ��������ľ�����׼
			//���벢����ͼ�񣬶���ͼ���Ӧ������
			cv::warpPerspective(srcImage, dstImage, homography.inv(), dstImage.size(), CV_INTER_CUBIC);
			s.psnr = PSNR(dstImage, gray);//���ص���double���� Ҫ�������ǻҶ�ͼ��
			//�����psnr��ôһֱ��0??���жϡ���

			

			if (0 && error >= 1)//0&&??
			{
				std::cout << "H expected:" << expectedHomography << std::endl;
				std::cout << "H actual:" << homography << std::endl;
				std::cout << "H error:" << error << std::endl;
				std::cout << "R error:" << s.reprojectionError(0) << ";"
					<< s.reprojectionError(1) << ";"
					<< s.reprojectionError(2) << ";"
					<< s.reprojectionError(3) << std::endl;

				cv::Mat matchesImg;
				cv::drawMatches(transformedImage,
					resKpReal,
					gray,
					sourceKp,
					inliermatches,
					matchesImg,
					cv::Scalar::all(-1),
					cv::Scalar::all(-1),
					std::vector<char>(),
					cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
				//��������û��int�͵�correctmatches http://blog.sina.com.cn/sblog_a98e39a201017pgn.html
				cv::imshow("Matches", matchesImg);
				cv::waitKey(-1);
			}
		}

    }
    
    return true;
}

cv::Scalar computeReprojectionError(const Keypoints& source, const Keypoints& query, const Matches& matches, const cv::Mat& homography)
{
    assert(matches.size() > 0);

    const int pointsCount = matches.size();
    std::vector<cv::Point2f> srcPoints, dstPoints;
    std::vector<float> distances;

    for (int i = 0; i < pointsCount; i++)
    {
        srcPoints.push_back(source[matches[i].trainIdx].pt);//��ƥ��Ե�������ѹ��srcPoints��
        dstPoints.push_back(query[matches[i].queryIdx].pt);// //trainIdxΪtrain�����ӵ�������match�����к�����Ǹ�������  
    }

    cv::perspectiveTransform(dstPoints, dstPoints, homography.inv());//��������ı任�����������任��һ������ϵ�У�Ҫ����׼����Ҫ����  ���벢����ͼ�񣬶���ͼ���Ӧ������
    for (int i = 0; i < pointsCount; i++)
    {
        const cv::Point2f& src = srcPoints[i];
        const cv::Point2f& dst = dstPoints[i];

        cv::Point2f v = src - dst;//ƥ����е�������֮��
        distances.push_back(sqrtf(v.dot(v)));//���ÿһ��������ľ��룬��Ϊ�˼��������׼�ı�׼���������Ҷ�Ӧ��
    }

    
    cv::Scalar mean, dev;
    cv::meanStdDev(distances, mean, dev);

    cv::Scalar result;
    result(0) = mean(0);
    result(1) = dev(0);
    result(2) = *std::max_element(distances.begin(), distances.end());
    result(3) = *std::min_element(distances.begin(), distances.end());
    return result;
}
