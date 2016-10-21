#include "All.h"
#include <stack>
#include <map>

mmImages::mmImagesCalculationMethodI::sCalculationMethodParams const params =
{
	L"All",
	L"{D73EC4EE-5B64-412C-8C15-F9524E12256E}",
	L"Counts sum of odd dots thrown on yellow dices. The result is displayed as yellow rectangles.",
	false,
	{ 0, 0 },
	{ 0, L"Krzysztof", L"Bigaj", L"" },
	L""
};


All::All() :
create_new_image(false), perspective(false)
{
	m_sCMParams = params;
	BindInputParam(L"Image name", mmImages::mmGenericParamI::mmImageNameType, m_image_name);
	BindInputParam(L"Create new image?", mmImages::mmGenericParamI::mmBoolType, create_new_image);
	BindInputParam(L"Is image perspective?", mmImages::mmGenericParamI::mmBoolType, perspective);
}

bool All::Calculate(){
	if (initialize()){
		if (perspective){
			rgbToHsv();
			channelThresholding(0.17, 0.10, 0.85, 0.5, 0.82, 0.41);
			binarize();
			medianFilter(6);
			dilation(2);
			medianFilter(1);
		}
		else{
			rgbToHsv();
			channelThresholding(0.17, 0.10, 0.85, 0.7, 0.82, 0.30);
			medianFilter(8);
			binarize();
			dilation(3);
			erosion(1);
		}
		objectLabeling(400);
		dotsLabeling(160000);
		dotsToDiceMatching();
		drawRect(countResult());

		finalize();
		return true;
	}
	
	return false;
}


bool All::initialize(){
	//we load image from GUI
	img = m_psImageStructure->FindImage(NULL, m_image_name);
	if (!img){
		return false;
	}

	//we get loaded image parameters
	pixel_type = img->GetPixelType();
	if (pixel_type != 3){ // number of channels: 3 means mmP24 (RGB)
		return false;
	}
	width = img->GetWidth();
	height = img->GetHeight();

	//we create new empty image (it will be used as an output image)
	m_new_image_name = m_image_name;
	m_new_image_name += L"_new";
	m_new_image = m_psImageStructure->CreateImage(m_new_image_name, width, height, pixel_type);
	if (!m_new_image){
		return false;
	}

	//we initialize variables for loaded image
	pixelsCh1 = new mmReal[width * height];
	pixelsCh2 = new mmReal[width * height];
	pixelsCh3 = new mmReal[width * height];

	ch1 = img->GetChannel(0);
	ch2 = img->GetChannel(1);
	ch3 = img->GetChannel(2);

	ch1->GetRows(0, height, pixelsCh1);
	ch2->GetRows(0, height, pixelsCh2);
	ch3->GetRows(0, height, pixelsCh3);

	//we initialize variables for new image
	new_ch1 = m_new_image->GetChannel(0);
	new_ch2 = m_new_image->GetChannel(1);
	new_ch3 = m_new_image->GetChannel(2);

	output_pixelsCh1 = new mmReal[width * height];
	output_pixelsCh2 = new mmReal[width * height];
	output_pixelsCh3 = new mmReal[width * height];

	return true;
}

void All::rgbToHsv(){
	mmReal hue;
	mmReal value;
	mmReal saturation;
	mmReal temp;

	//conversion algorithm
	for (mmInt y = 0; y < height; ++y){
		for (mmInt x = 0; x < width; ++x){
			temp = std::min(std::min(pixelsCh1[y * width + x], pixelsCh2[y * width + x]), pixelsCh3[y * width + x]);
			value = std::max(std::max(pixelsCh1[y * width + x], pixelsCh2[y * width + x]), pixelsCh3[y * width + x]);
			if (temp == value){
				hue = 0.0;
			}
			else{
				if (pixelsCh1[y * width + x] == value){
					hue = 0.0 + (pixelsCh2[y * width + x] - pixelsCh3[y * width + x]) * 60 / (value - temp);
				}
				if (pixelsCh2[y * width + x] == value){
					hue = 120.0 + (pixelsCh3[y * width + x] - pixelsCh1[y * width + x]) * 60 / (value - temp);
				}
				if (pixelsCh3[y * width + x] == value){
					hue = 240.0 + (pixelsCh1[y * width + x] - pixelsCh2[y * width + x]) * 60 / (value - temp);
				}
			}
			if (hue < 0){
				hue = hue + 360;
			}
			hue = hue / 360.0;

			if (value == 0){
				saturation = 0;
			}
			else{
				saturation = (value - temp) / value;
			}
			pixelsCh1[y * width + x] = hue;
			pixelsCh2[y * width + x] = saturation;
			pixelsCh3[y * width + x] = value;
		}
	}
}

void All::channelThresholding(
	mmReal m_threshold_high_ch1,
	mmReal m_threshold_low_ch1,
	mmReal m_threshold_high_ch2,
	mmReal m_threshold_low_ch2,
	mmReal m_threshold_high_ch3,
	mmReal m_threshold_low_ch3)
{
	//channel thresholding algorithm
	for (mmInt y = 0; y < height; ++y){
		for (mmInt x = 0; x < width; ++x){
			pixelsCh1[y * width + x] = mmReal(pixelsCh1[y * width + x] > m_threshold_low_ch1 && pixelsCh1[y * width + x] < m_threshold_high_ch1);
			pixelsCh2[y * width + x] = mmReal(pixelsCh2[y * width + x] > m_threshold_low_ch2 && pixelsCh2[y * width + x] < m_threshold_high_ch2);
			pixelsCh3[y * width + x] = mmReal(pixelsCh3[y * width + x] > m_threshold_low_ch3 && pixelsCh3[y * width + x] < m_threshold_high_ch3);
		}
	}
}

void All::binarize(){
	//binarization algorithm
	for (mmInt y = 0; y < height; ++y){
		for (mmInt x = 0; x < width; ++x){
			if (pixelsCh1[y * width + x] != 1.0 || pixelsCh2[y * width + x] != 1.0 || pixelsCh3[y * width + x] != 1.0){
				pixelsCh1[y * width + x] = 0.0;
				pixelsCh2[y * width + x] = 0.0;
				pixelsCh3[y * width + x] = 0.0;
			}
		}
	}
}

void All::medianFilter(mmInt number_of_repeats){
	mmInt count = 0;

	//median filter algorithm
	for (mmInt r = 0; r < number_of_repeats; ++r){
		for (mmInt y = 1; y < (height - 1); ++y){
			for (mmInt x = 1; x < (width - 1); ++x){
				count = 0;
				count += pixelsCh1[y * width + x]; //middle
				count += pixelsCh1[y * width + x + 1]; //right
				count += pixelsCh1[y * width + x - 1]; //left
				count += pixelsCh1[(y + 1) * width + x]; //top
				count += pixelsCh1[(y - 1) * width + x]; //bottom
				count += pixelsCh1[(y + 1) * width + x + 1]; //top-right
				count += pixelsCh1[(y + 1) * width + x - 1]; //top-left
				count += pixelsCh1[(y - 1) * width + x + 1]; //bottom-right
				count += pixelsCh1[(y - 1) * width + x - 1]; //bottom-left
				if (count >= 5){
					output_pixelsCh1[y * width + x] = 1.0;
				}
				else{
					output_pixelsCh1[y * width + x] = 0.0;
				}

				count = 0;
				count += pixelsCh2[y * width + x]; //middle
				count += pixelsCh2[y * width + x + 1]; //right
				count += pixelsCh2[y * width + x - 1]; //left
				count += pixelsCh2[(y + 1) * width + x]; //top
				count += pixelsCh2[(y - 1) * width + x]; //bottom
				count += pixelsCh2[(y + 1) * width + x + 1]; //top-right
				count += pixelsCh2[(y + 1) * width + x - 1]; //top-left
				count += pixelsCh2[(y - 1) * width + x + 1]; //bottom-right
				count += pixelsCh2[(y - 1) * width + x - 1]; //bottom-left
				if (count >= 5){
					output_pixelsCh2[y * width + x] = 1.0;
				}
				else{
					output_pixelsCh2[y * width + x] = 0.0;
				}

				count = 0;
				count += pixelsCh3[y * width + x]; //middle
				count += pixelsCh3[y * width + x + 1]; //right
				count += pixelsCh3[y * width + x - 1]; //left
				count += pixelsCh3[(y + 1) * width + x]; //top
				count += pixelsCh3[(y - 1) * width + x]; //bottom
				count += pixelsCh3[(y + 1) * width + x + 1]; //top-right
				count += pixelsCh3[(y + 1) * width + x - 1]; //top-left
				count += pixelsCh3[(y - 1) * width + x + 1]; //bottom-right
				count += pixelsCh3[(y - 1) * width + x - 1]; //bottom-left
				if (count >= 5){
					output_pixelsCh3[y * width + x] = 1.0;
				}
				else{
					output_pixelsCh3[y * width + x] = 0.0;
				}
			}
		}
		memcpy(pixelsCh1, output_pixelsCh1, width * height * sizeof(mmReal));
		memcpy(pixelsCh2, output_pixelsCh2, width * height * sizeof(mmReal));
		memcpy(pixelsCh3, output_pixelsCh3, width * height * sizeof(mmReal));
	}
}

void All::dilation(mmInt number_of_repeats){
	bool hasNeighbour = false;

	//dilation algorithm
	for (mmInt r = 0; r < number_of_repeats; ++r){
		for (mmInt y = 1; y < (height - 1); ++y){
			for (mmInt x = 1; x < (width - 1); ++x){
				//Ch1
				hasNeighbour = false;
				if (pixelsCh1[y * width + x]) //middle
					hasNeighbour = true;
				if (pixelsCh1[y * width + x + 1]) //right
					hasNeighbour = true;
				if (pixelsCh1[y * width + x - 1]) //left
					hasNeighbour = true;
				if (pixelsCh1[(y + 1) * width + x]) //top
					hasNeighbour = true;
				if (pixelsCh1[(y - 1) * width + x]) //bottom
					hasNeighbour = true;
				if (pixelsCh1[(y + 1) * width + x + 1]) //top-right
					hasNeighbour = true;
				if (pixelsCh1[(y + 1) * width + x - 1]) //top-left
					hasNeighbour = true;
				if (pixelsCh1[(y - 1) * width + x + 1]) //bottom-right
					hasNeighbour = true;
				if (pixelsCh1[(y - 1) * width + x - 1]) //bottom-left
					hasNeighbour = true;

				if (hasNeighbour){
					output_pixelsCh1[y * width + x] = 1.0;
				}
				else{
					output_pixelsCh1[y * width + x] = 0.0;
				}

				//Ch2
				hasNeighbour = false;
				if (pixelsCh2[y * width + x]) //middle
					hasNeighbour = true;
				if (pixelsCh2[y * width + x + 1]) //right
					hasNeighbour = true;
				if (pixelsCh2[y * width + x - 1]) //left
					hasNeighbour = true;
				if (pixelsCh2[(y + 1) * width + x]) //top
					hasNeighbour = true;
				if (pixelsCh2[(y - 1) * width + x]) //bottom
					hasNeighbour = true;
				if (pixelsCh2[(y + 1) * width + x + 1]) //top-right
					hasNeighbour = true;
				if (pixelsCh2[(y + 1) * width + x - 1]) //top-left
					hasNeighbour = true;
				if (pixelsCh2[(y - 1) * width + x + 1]) //bottom-right
					hasNeighbour = true;
				if (pixelsCh2[(y - 1) * width + x - 1]) //bottom-left
					hasNeighbour = true;

				if (hasNeighbour){
					output_pixelsCh2[y * width + x] = 1.0;
				}
				else{
					output_pixelsCh2[y * width + x] = 0.0;
				}

				//Ch3
				hasNeighbour = false;
				if (pixelsCh3[y * width + x]) //middle
					hasNeighbour = true;
				if (pixelsCh3[y * width + x + 1]) //right
					hasNeighbour = true;
				if (pixelsCh3[y * width + x - 1]) //left
					hasNeighbour = true;
				if (pixelsCh3[(y + 1) * width + x]) //top
					hasNeighbour = true;
				if (pixelsCh3[(y - 1) * width + x]) //bottom
					hasNeighbour = true;
				if (pixelsCh3[(y + 1) * width + x + 1]) //top-right
					hasNeighbour = true;
				if (pixelsCh3[(y + 1) * width + x - 1]) //top-left
					hasNeighbour = true;
				if (pixelsCh3[(y - 1) * width + x + 1]) //bottom-right
					hasNeighbour = true;
				if (pixelsCh3[(y - 1) * width + x - 1]) //bottom-left
					hasNeighbour = true;

				if (hasNeighbour){
					output_pixelsCh3[y * width + x] = 1.0;
				}
				else{
					output_pixelsCh3[y * width + x] = 0.0;
				}
			}
		}
		memcpy(pixelsCh1, output_pixelsCh1, width * height * sizeof(mmReal));
		memcpy(pixelsCh2, output_pixelsCh2, width * height * sizeof(mmReal));
		memcpy(pixelsCh3, output_pixelsCh3, width * height * sizeof(mmReal));
	}
}

void All::erosion(mmInt number_of_repeats){
	bool hasEmptyNeighbour = false;

	//erosion algorithm
	for (mmInt r = 0; r < number_of_repeats; ++r){
		for (mmInt y = 1; y < (height - 1); ++y){
			for (mmInt x = 1; x < (width - 1); ++x){
				//ch1
				hasEmptyNeighbour = false;
				if (!pixelsCh1[y * width + x]) //middle
					hasEmptyNeighbour = true;
				if (!pixelsCh1[y * width + x + 1]) //right
					hasEmptyNeighbour = true;
				if (!pixelsCh1[y * width + x - 1]) //left
					hasEmptyNeighbour = true;
				if (!pixelsCh1[(y + 1) * width + x]) //top
					hasEmptyNeighbour = true;
				if (!pixelsCh1[(y - 1) * width + x]) //bottom
					hasEmptyNeighbour = true;
				if (!pixelsCh1[(y + 1) * width + x + 1]) //top-right
					hasEmptyNeighbour = true;
				if (!pixelsCh1[(y + 1) * width + x - 1]) //top-left
					hasEmptyNeighbour = true;
				if (!pixelsCh1[(y - 1) * width + x + 1]) //bottom-right
					hasEmptyNeighbour = true;
				if (!pixelsCh1[(y - 1) * width + x - 1]) //bottom-left
					hasEmptyNeighbour = true;

				if (hasEmptyNeighbour){
					output_pixelsCh1[y * width + x] = 0.0;
				}
				else{
					output_pixelsCh1[y * width + x] = 1.0;
				}


				//ch2
				hasEmptyNeighbour = false;
				if (!pixelsCh2[y * width + x]) //middle
					hasEmptyNeighbour = true;
				if (!pixelsCh2[y * width + x + 1]) //right
					hasEmptyNeighbour = true;
				if (!pixelsCh2[y * width + x - 1]) //left
					hasEmptyNeighbour = true;
				if (!pixelsCh2[(y + 1) * width + x]) //top
					hasEmptyNeighbour = true;
				if (!pixelsCh2[(y - 1) * width + x]) //bottom
					hasEmptyNeighbour = true;
				if (!pixelsCh2[(y + 1) * width + x + 1]) //top-right
					hasEmptyNeighbour = true;
				if (!pixelsCh2[(y + 1) * width + x - 1]) //top-left
					hasEmptyNeighbour = true;
				if (!pixelsCh2[(y - 1) * width + x + 1]) //bottom-right
					hasEmptyNeighbour = true;
				if (!pixelsCh2[(y - 1) * width + x - 1]) //bottom-left
					hasEmptyNeighbour = true;

				if (hasEmptyNeighbour){
					output_pixelsCh2[y * width + x] = 0.0;
				}
				else{
					output_pixelsCh2[y * width + x] = 1.0;
				}


				//ch3
				hasEmptyNeighbour = false;
				if (!pixelsCh3[y * width + x]) //middle
					hasEmptyNeighbour = true;
				if (!pixelsCh3[y * width + x + 1]) //right
					hasEmptyNeighbour = true;
				if (!pixelsCh3[y * width + x - 1]) //left
					hasEmptyNeighbour = true;
				if (!pixelsCh3[(y + 1) * width + x]) //top
					hasEmptyNeighbour = true;
				if (!pixelsCh3[(y - 1) * width + x]) //bottom
					hasEmptyNeighbour = true;
				if (!pixelsCh3[(y + 1) * width + x + 1]) //top-right
					hasEmptyNeighbour = true;
				if (!pixelsCh3[(y + 1) * width + x - 1]) //top-left
					hasEmptyNeighbour = true;
				if (!pixelsCh3[(y - 1) * width + x + 1]) //bottom-right
					hasEmptyNeighbour = true;
				if (!pixelsCh3[(y - 1) * width + x - 1]) //bottom-left
					hasEmptyNeighbour = true;

				if (hasEmptyNeighbour){
					output_pixelsCh3[y * width + x] = 0.0;
				}
				else{
					output_pixelsCh3[y * width + x] = 1.0;
				}
			}
		}
		memcpy(pixelsCh1, output_pixelsCh1, width * height * sizeof(mmReal));
		memcpy(pixelsCh2, output_pixelsCh2, width * height * sizeof(mmReal));
		memcpy(pixelsCh3, output_pixelsCh3, width * height * sizeof(mmReal));
	}
}

void All::objectLabeling(mmInt threshold){
	mmReal curLab = 0.2;
	std::stack<mmInt> mStack;
	mmInt count = 0;

	//clear channel 2
	for (mmInt y = 0; y < height; ++y){
		for (mmInt x = 0; x < width; ++x){
			pixelsCh2[y * width + x] = 0.0;
		}
	}

	//object labeling algorithm
	for (mmInt y = 0; y < height; ++y){
		for (mmInt x = 0; x < width; ++x){
			if (pixelsCh1[y * width + x] == 1.0 && pixelsCh2[y * width + x] == 0.0){
				pixelsCh2[y * width + x] = curLab;
				++count;
				mStack.push(y * width + x);

				while (!mStack.empty()){
					mmInt pos = mStack.top();
					mStack.pop();
					if (pixelsCh1[pos + 1] == 1.0 && pixelsCh2[pos + 1] == 0.0){ //right
						pixelsCh2[pos + 1] = curLab;
						++count;
						mStack.push(pos + 1);
					}
					if (pixelsCh1[pos - 1] == 1.0 && pixelsCh2[pos - 1] == 0.0){ //left
						pixelsCh2[pos - 1] = curLab;
						++count;
						mStack.push(pos - 1);
					}
					if (pixelsCh1[pos + width] == 1.0 && pixelsCh2[pos + width] == 0.0){ //bottom
						pixelsCh2[pos + width] = curLab;
						++count;
						mStack.push(pos + width);
					}
					if (pixelsCh1[pos - width] == 1.0 && pixelsCh2[pos - width] == 0.0){ //top
						pixelsCh2[pos - width] = curLab;
						++count;
						mStack.push(pos - width);
					}
					if (pixelsCh1[pos + width + 1] == 1.0 && pixelsCh2[pos + width + 1] == 0.0){ //bottom-right
						pixelsCh2[pos + width + 1] = curLab;
						++count;
						mStack.push(pos + width + 1);
					}
					if (pixelsCh1[pos + width - 1] == 1.0 && pixelsCh2[pos + width - 1] == 0.0){ //bottom-left
						pixelsCh2[pos + width - 1] = curLab;
						++count;
						mStack.push(pos + width - 1);
					}
					if (pixelsCh1[pos - width + 1] == 1.0 && pixelsCh2[pos - width + 1] == 0.0){ //top-right
						pixelsCh2[pos - width + 1] = curLab;
						++count;
						mStack.push(pos - width + 1);
					}
					if (pixelsCh1[pos - width - 1] == 1.0 && pixelsCh2[pos - width - 1] == 0.0){ //top-left
						pixelsCh2[pos - width - 1] = curLab;
						++count;
						mStack.push(pos - width - 1);
					}
				}

				//if an object is too small it is not a dice so we delete it
				if (count < threshold){
					mStack.push(y * width + x);

					while (!mStack.empty()){
						mmInt pos = mStack.top();
						mStack.pop();
						if (pixelsCh1[pos + 1] == 1.0 && pixelsCh2[pos + 1] == curLab){ //right
							pixelsCh2[pos + 1] = 0.0;
							mStack.push(pos + 1);
						}
						if (pixelsCh1[pos - 1] == 1.0 && pixelsCh2[pos - 1] == curLab){ //left
							pixelsCh2[pos - 1] = 0.0;
							mStack.push(pos - 1);
						}
						if (pixelsCh1[pos + width] == 1.0 && pixelsCh2[pos + width] == curLab){ //bottom
							pixelsCh2[pos + width] = 0.0;
							mStack.push(pos + width);
						}
						if (pixelsCh1[pos - width] == 1.0 && pixelsCh2[pos - width] == curLab){ //top
							pixelsCh2[pos - width] = 0.0;
							mStack.push(pos - width);
						}
						if (pixelsCh1[pos + width + 1] == 1.0 && pixelsCh2[pos + width + 1] == curLab){ //bottom-right
							pixelsCh2[pos + width + 1] = 0.0;
							mStack.push(pos + width + 1);
						}
						if (pixelsCh1[pos + width - 1] == 1.0 && pixelsCh2[pos + width - 1] == curLab){ //bottom-left
							pixelsCh2[pos + width - 1] = 0.0;
							mStack.push(pos + width - 1);
						}
						if (pixelsCh1[pos - width + 1] == 1.0 && pixelsCh2[pos - width + 1] == curLab){ //top-right
							pixelsCh2[pos - width + 1] = 0.0;
							mStack.push(pos - width + 1);
						}
						if (pixelsCh1[pos - width - 1] == 1.0 && pixelsCh2[pos - width - 1] == curLab){ //top-left
							pixelsCh2[pos - width - 1] = 0.0;
							mStack.push(pos - width - 1);
						}
					}
				}
				else{
					curLab = curLab + 0.1;
				}
				count = 0;
			}
		}
	}
}


void All::dotsLabeling(mmInt threshold){
	mmReal curLab = 0.2;
	mmReal backgroundLab;
	std::stack<mmInt> mStack;
	mmInt count = 0;
	
	//clear channel 3
	for (mmInt y = 0; y < height; ++y){
		for (mmInt x = 0; x < width; ++x){
			pixelsCh3[y * width + x] = 0.0;
		}
	}

	//dots labeling algorithm
	for (mmInt y = 0; y < height; ++y){
		for (mmInt x = 0; x < width; ++x){
			if (pixelsCh1[y * width + x] == 0.0 && pixelsCh3[y * width + x] == 0.0){
				pixelsCh3[y * width + x] = curLab;
				++count;
				mStack.push(y * width + x);

				while (!mStack.empty()){
					mmInt pos = mStack.top();
					mStack.pop();
					if (pixelsCh1[pos + 1] == 0.0 && pixelsCh3[pos + 1] == 0.0){ //right
						pixelsCh3[pos + 1] = curLab;
						++count;
						mStack.push(pos + 1);
					}
					if (pixelsCh1[pos - 1] == 0.0 && pixelsCh3[pos - 1] == 0.0){ //left
						pixelsCh3[pos - 1] = curLab;
						++count;
						mStack.push(pos - 1);
					}
					if (pixelsCh1[pos + width] == 0.0 && pixelsCh3[pos + width] == 0.0){ //bottom
						pixelsCh3[pos + width] = curLab;
						++count;
						mStack.push(pos + width);
					}
					if (pixelsCh1[pos - width] == 0.0 && pixelsCh3[pos - width] == 0.0){ //top
						pixelsCh3[pos - width] = curLab;
						++count;
						mStack.push(pos - width);
					}
					if (pixelsCh1[pos + width + 1] == 0.0 && pixelsCh3[pos + width + 1] == 0.0){ //bottom-right
						pixelsCh3[pos + width + 1] = curLab;
						++count;
						mStack.push(pos + width + 1);
					}
					if (pixelsCh1[pos + width - 1] == 0.0 && pixelsCh3[pos + width - 1] == 0.0){ //bottom-left
						pixelsCh3[pos + width - 1] = curLab;
						++count;
						mStack.push(pos + width - 1);
					}
					if (pixelsCh1[pos - width + 1] == 0.0 && pixelsCh3[pos - width + 1] == 0.0){ //top-right
						pixelsCh3[pos - width + 1] = curLab;
						++count;
						mStack.push(pos - width + 1);
					}
					if (pixelsCh1[pos - width - 1] == 0.0 && pixelsCh3[pos - width - 1] == 0.0){ //top-left
						pixelsCh3[pos - width - 1] = curLab;
						++count;
						mStack.push(pos - width - 1);
					}
				}

				//if an object is too big it is a background so we delete it later
				if (count > threshold){
					backgroundLab = curLab;
				}

				curLab = curLab + 0.03;
				count = 0;
			}
		}
	}

	for (mmInt y = 0; y < height; ++y){
		for (mmInt x = 0; x < width; ++x){
			if (pixelsCh1[y * width + x] == 0.0 && pixelsCh3[y * width + x] == backgroundLab){
				pixelsCh3[y * width + x] = 0.0;
			}
		}
	}
}

void All::dotsToDiceMatching(){
	//clear channel 1
	for (mmInt y = 0; y < height; ++y){
		for (mmInt x = 0; x < width; ++x){
			pixelsCh1[y * width + x] = 0.0;
		}
	}

	//matching algorithm
	for (mmInt y = 0; y < height; ++y){
		for (mmInt x = 0; x < width; ++x){
			if (pixelsCh3[y * width + x] != 0.0){
				mmInt pos = y * width + x;
				while (pixelsCh2[pos] == 0.0){
					++pos;
				}
				pixelsCh1[y * width + x] = pixelsCh2[pos];
			}
		}
	}
}


mmInt All::countResult(){
	std::map <int, bool> wasCounted;
	for (int i = 20; i < 100; i = i + 3){
		wasCounted[i] = false;
	}

	std::map <int, int> diceRoll;
	for (int i = 20; i < 100; i = i + 10){
		diceRoll[i] = 0;
	}

	//counting algorithm
	for (mmInt y = 0; y < height; ++y){
		for (mmInt x = 0; x < width; ++x){
			if (pixelsCh3[y * width + x] != 0.0){
				if (!wasCounted[int(pixelsCh3[y * width + x] * 100.0)]){
					diceRoll[int(pixelsCh1[y * width + x] * 100.0)] = diceRoll[int(pixelsCh1[y * width + x] * 100.0)] + 1;
					wasCounted[int(pixelsCh3[y * width + x] * 100.0)] = true;
				}
			}
		}
	}

	//sum odd results
	mmInt sum = 0;
	for (int i = 30; i < 100; i = i + 10){
		if (diceRoll[i] % 2 == 1){
			sum += mmInt(diceRoll[i]);
		}
	}
	return sum;
}

void All::drawRect(mmInt count){
	mmInt startPos = 50;
	mmInt size = 50;
	for (mmInt i = 0; i < count; ++i){
		for (mmInt y = startPos; y < startPos + size; ++y){
			for (mmInt x = startPos + startPos*2*i; x < startPos + startPos*2*i + size; ++x){
				pixelsCh1[y * width + x] = 1.0;
				pixelsCh2[y * width + x] = 1.0;
			}
		}
	}
}

void All::finalize(){
	if (create_new_image){
		//we set values to the new image
		new_ch1->SetRows(0, height, pixelsCh1);
		new_ch2->SetRows(0, height, pixelsCh2);
		new_ch3->SetRows(0, height, pixelsCh3);
		
	}
	else{
		//we overwrite the old image
		ch1->SetRows(0, height, pixelsCh1);
		ch2->SetRows(0, height, pixelsCh2);
		ch3->SetRows(0, height, pixelsCh3);
		m_psImageStructure->DeleteImage(m_new_image->GetID());
	}
	
	delete[] pixelsCh1;
	delete[] pixelsCh2;
	delete[] pixelsCh3;

	delete[] output_pixelsCh1;
	delete[] output_pixelsCh2;
	delete[] output_pixelsCh3;
}