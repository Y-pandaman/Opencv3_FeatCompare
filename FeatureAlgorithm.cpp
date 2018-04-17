#include "FeatureAlgorithm.hpp"
#include <cassert>



static cv::Ptr<cv::flann::IndexParams> indexParamsForDescriptorType(int descriptorType, int defaultNorm)
{
    switch (defaultNorm)
    {
    case cv::NORM_L2:
        return cv::Ptr<cv::flann::IndexParams>(new cv::flann::KDTreeIndexParams());

    case cv::NORM_HAMMING:
        return cv::Ptr<cv::flann::IndexParams>(new cv::flann::LshIndexParams(20, 15, 2));

    default:
        CV_Assert(false && "Unsupported descriptor type");
    };
}

cv::Ptr<cv::DescriptorMatcher> matcherForDescriptorType(int descriptorType, int defaultNorm, bool bruteForce)
{
    if (bruteForce)
        return cv::Ptr<cv::DescriptorMatcher>(new cv::BFMatcher(defaultNorm, true));
    else
        return  cv::Ptr<cv::DescriptorMatcher>(new cv::FlannBasedMatcher(indexParamsForDescriptorType(descriptorType, defaultNorm)));
}

FeatureAlgorithm::FeatureAlgorithm(const std::string& n, cv::Ptr<cv::FeatureDetector> d, cv::Ptr<cv::DescriptorExtractor> e, bool useBruteForceMather)
: name(n)
, knMatchSupported(false)
, detector(d)//�����
, extractor(e)//��ȡ����
, matcher(matcherForDescriptorType(e->descriptorSize(), e->defaultNorm(), useBruteForceMather))
{
    CV_Assert(d);
    CV_Assert(e);
}

FeatureAlgorithm::FeatureAlgorithm(const std::string& n, cv::Ptr<cv::Feature2D> fe, bool useBruteForceMather)
: name(n)
, knMatchSupported(false)
, featureEngine(fe)//���캯��������������cv::Ptr<cv::Feature2D>           featureEngine;Feature2D�̳��������㷨
, matcher(matcherForDescriptorType(fe->descriptorSize(), fe->defaultNorm(), useBruteForceMather))
{
    CV_Assert(fe);
}


bool FeatureAlgorithm::extractFeatures(const cv::Mat& image, Keypoints& kp, Descriptors& desc) const
{
    assert(!image.empty());

    if (featureEngine)//֮ǰ��FeatureAlgorithm���������أ��������featureEngine��ֱ�ӵõ�������
    {
        //(*featureEngine)(image, cv::noArray(), kp, desc);
		featureEngine->detectAndCompute(image, cv::noArray(), kp, desc);
    }
    else
    {
        detector->detect(image, kp);//���û��featureEngune���ȼ������ȡ
    
        if (kp.empty())
            return false;
    
        extractor->compute(image, kp, desc);
    }
    
    
    return kp.size() > 0;
}

void FeatureAlgorithm::matchFeatures(const Descriptors& train, const Descriptors& query, Matches& matches) const
{
    matcher->match(query, train, matches);
}

void FeatureAlgorithm::matchFeatures(const Descriptors& train, const Descriptors& query, int k, std::vector<Matches>& matches) const//matchFeatures�����أ��ֱ�������ٺ�knn��������int��k����knn
{
    assert(knMatchSupported);
    matcher->knnMatch(query, train, matches, k);
}

