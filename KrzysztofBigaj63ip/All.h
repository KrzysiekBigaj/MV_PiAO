#pragma once

#include <plugin/mmCalcMethod.h>

class All : public mmImages::mmCalcMethod{
public:
	All();
	bool Calculate();

private:
	//input parameters
	mmString m_image_name;
	bool create_new_image;
	bool perspective;

	//general variables
	mmImages::mmImageI * img;
	mmImages::mmImageI * m_new_image;
	mmString m_new_image_name;
	mmImages::mmImageI::mmPixelType pixel_type;
	mmInt width;
	mmInt height;
	mmReal * pixelsCh1;
	mmReal * pixelsCh2;
	mmReal * pixelsCh3;
	mmReal * output_pixelsCh1;
	mmReal * output_pixelsCh2;
	mmReal * output_pixelsCh3;
	mmImages::mmLayerI * ch1;
	mmImages::mmLayerI * ch2;
	mmImages::mmLayerI * ch3;
	mmImages::mmLayerI * new_ch1;
	mmImages::mmLayerI * new_ch2;
	mmImages::mmLayerI * new_ch3;

	//methods
	bool initialize();
	void rgbToHsv();
	void channelThresholding(
		mmReal m_threshold_high_ch1,
		mmReal m_threshold_low_ch1,
		mmReal m_threshold_high_ch2,
		mmReal m_threshold_low_ch2,
		mmReal m_threshold_high_ch3,
		mmReal m_threshold_low_ch3			
	);
	void binarize();
	void medianFilter(mmInt number_of_repeats);
	void dilation(mmInt number_of_repeats);
	void erosion(mmInt number_of_repeats);
	void objectLabeling(mmInt threshold);
	void dotsLabeling(mmInt threshold);
	void dotsToDiceMatching();
	mmInt countResult();
	void drawRect(mmInt count);
	void finalize();
};