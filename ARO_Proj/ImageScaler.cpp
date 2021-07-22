#include "stdafx.h"
#include "ImageScaler.h"

#include <algorithm>
#include <iostream>

// Constructor
// @param output_image_width -> x diminsion size of output image
// @param output_image_height -> y diminsion size of output image
// @param output_image_depth -> z diminsion size of output image
ImageScaler::ImageScaler(int output_image_width, int output_image_height, int output_image_depth, unsigned char* wavefront_correction_hack) :
output_image_width_(output_image_width),
output_image_height_(output_image_height),
output_image_depth_(output_image_depth),
wavefront_correction_hack_(wavefront_correction_hack)
{
	bin_size_x_ = bin_size_y_ = -1;
	max_bins_x_ = max_bins_y_ = -1;
	used_bins_x_ = used_bins_y_ = -1;
	remainder_x_ = remainder_y_ = -1;
	left_remainder_x_ = top_remainder_y_ = -1;
	lut_ = 0;
	requirement_set_bin_size_ = requirement_set_used_bins_ = requirement_set_lut_ = false;
}

ImageScaler::~ImageScaler() {
	delete this->lut_;
	delete this->wavefront_correction_hack_;
}

// Set bin size
// @param bin_size_x -> x dimension size of bins
// @param bin_size_y -> y dimension size of bins
void ImageScaler::SetBinSize(int bin_size_x, int bin_size_y)
{
	bin_size_x_ = bin_size_x;
	bin_size_y_ = bin_size_y;
	max_bins_x_ = output_image_width_ / bin_size_x;
	max_bins_y_ = output_image_height_ / bin_size_y;
	remainder_x_ = output_image_width_ % bin_size_x;
	remainder_y_ = output_image_height_ % bin_size_y;
	requirement_set_bin_size_ = true;
}

// Get the maximum number of bins based on image size and bin size
// @param max_bins_x -> output to be set with max bin numbers in the x dimension
// @param max_bins_y -> output to be set with max bin numbers in the y dimension
void ImageScaler::GetMaxBins(int &max_bins_x, int &max_bins_y)
{
	max_bins_x = max_bins_x_;
	max_bins_y = max_bins_y_;
}

// Sets the number of bins that there will be in the x and y dimensions
// @param used_bins_x -> the number of bins in the x dimension
// @param used_bins_y -> the number of bins in the y dimension
void ImageScaler::SetUsedBins(int used_bins_x, int used_bins_y)
{
	used_bins_x_ = max(0, min(used_bins_x, max_bins_x_));
	used_bins_y_ = max(0, min(used_bins_y, max_bins_y_));
	remainder_x_ = (output_image_width_ % bin_size_x_) + ((max_bins_x_ - used_bins_x_) * bin_size_x_);
	remainder_y_ = (output_image_height_ % bin_size_y_) + ((max_bins_y_ - used_bins_y_) * bin_size_y_);
	left_remainder_x_ = remainder_x_ / 2;
	top_remainder_y_ = (remainder_y_ / 2) *output_image_width_;
	requirement_set_used_bins_ = true;
}

// Gets the total number of bins
int ImageScaler::GetTotalBinNum()
{
	return used_bins_x_ * used_bins_y_;
}

// Sets the LUT to be used during scaling
// @param lut -> the LUT to use
void ImageScaler::SetLUT(unsigned short* lut)
{
	lut_ = lut;
	requirement_set_lut_ = true;
}

// Puts a value of zero into every bin in an image
// @param output_image -> the image to be zeroed
void ImageScaler::ZeroOutputImage(unsigned char* output_image)
{
	for (int i = 0; i < output_image_depth_*output_image_height_*output_image_width_; i++)
	{
		output_image[i] = 0;
	}
}

// Takes an array holding values for each bin and fills an image with those values
// @param input_image -> the array holding all the bin values
// @param output_image -> the array holding the output image
void ImageScaler::TranslateImage(int* input_image, unsigned char* output_image)
{
	if (requirement_set_bin_size_ && requirement_set_used_bins_ && requirement_set_lut_)
	{	// prevent action if all steps to set up image scaling have not been completed
		int start_point = top_remainder_y_ + left_remainder_x_;
		for (int i = 0; i < used_bins_y_; i++)
		{	// for each row
			int line_start_point = start_point + (i*(bin_size_y_*output_image_width_));
			for (int j = 0; j < used_bins_x_; j++)
			{	// for each bin in the row
				int bin_start_point = line_start_point + (j * bin_size_x_);
				int pix_value;
				//if (lut_ != NULL)
				//{
					//pix_value = lut_[(input_image[(i * used_bins_x_) + j] << 8)];
				//}
				//else
				//{
					pix_value = (input_image[(i * used_bins_x_) + j]);
				//}
				for (int k = 0; k < bin_size_y_; k++)
				{	// for each line in each bin
					int write_start_point = bin_start_point + (k * output_image_width_);
					for (int l = 0; l < bin_size_x_; l++)
					{	// for each space in each line
						int write_point = write_start_point + l;
						output_image[write_point*output_image_depth_] = (unsigned char)pix_value;
						if (output_image_depth_ > 1)
						{
							output_image[(write_point*output_image_depth_) + 1] = (pix_value >> 8);
						}
					}
				}
			}
		}
	}
}

// Takes an array holding values for each bin and fills an image with those values
// @param input_image -> the array holding all the bin values
// @param output_image -> the array holding the output image	
void ImageScaler::TranslateImage(std::vector<int> * input_image, unsigned char* output_image) {
	if (requirement_set_bin_size_ && requirement_set_used_bins_ && requirement_set_lut_) {
		// prevent action if all steps to set up image scaling have not been completed
		int start_point = top_remainder_y_ + left_remainder_x_;
		for (int i = 0; i < used_bins_y_; i++) {	// for each row
			
			int line_start_point = start_point + (i*(bin_size_y_*output_image_width_));
			for (int j = 0; j < used_bins_x_; j++) {	// for each bin in the row
				int bin_start_point = line_start_point + (j * bin_size_x_);
				int pix_value;
				//if (lut_ != NULL)
				//{
				//	pix_value = lut_[((*input_image)[(i * used_bins_x_) + j] << 8)];
				//}
				//else
				//{
					pix_value = ((*input_image)[(i * used_bins_x_) + j]);
				//}
				for (int k = 0; k < bin_size_y_; k++)
				{	// for each line in each bin
					int write_start_point = bin_start_point + (k * output_image_width_);
					for (int l = 0; l < bin_size_x_; l++)
					{	// for each space in each line
						int write_point = write_start_point + l;
						output_image[write_point*output_image_depth_] = (unsigned char)pix_value;
						if (output_image_depth_ > 1)
						{
							output_image[(write_point*output_image_depth_) + 1] = (pix_value >> 8);
						}
					}
				}
			}
		}
	}
}

// Updates the value of one bin in an image
// @param image -> the image to be updated
// @param x_bin -> the x indice of the bin to be updated
// @param y_bin -> the y indice of the bin to be updated
// @param value -> the value to set the bin to
void ImageScaler::UpdateSelectedBin(unsigned char* image, int x_bin, int y_bin, int value)
{
	if (requirement_set_bin_size_ && requirement_set_used_bins_ && requirement_set_lut_) {
		int pix_value;
		//if (lut_ != NULL)
		//{
		//	pix_value = lut_[(value << 8)];
		//}
		//else
		//{
			pix_value = (value << 8);
		//}
		int bin_start_point = top_remainder_y_ + left_remainder_x_;
		bin_start_point += bin_size_y_ * output_image_width_ * y_bin;
		bin_start_point += bin_size_x_ * x_bin;
		for (int k = 0; k < bin_size_y_; k++)
		{	// for each line in each bin
			int write_start_point = bin_start_point + (k * output_image_width_);
			for (int l = 0; l < bin_size_x_; l++)
			{	// for each space in each line
				int write_point = write_start_point + l;
				image[write_point*output_image_depth_] = (unsigned char)pix_value;
				image[(write_point*output_image_depth_) + 1] = (pix_value >> 8);
			}
		}
	}
}

// Updateds a bin value in an image
// @param image -> the image to be updated
// @param bin -> the bin to be updated, the value wraps across rows
// @param value -> the value to set the bin to
void ImageScaler::UpdateSelectedBin(unsigned char* image, int bin, int value)
{
	if (requirement_set_bin_size_ && requirement_set_used_bins_ && requirement_set_lut_) {
		int x_bin = bin % used_bins_x_;
		int y_bin = bin / used_bins_x_;
		int pix_value;
		//if (lut_ != NULL)
		//{
			//pix_value = lut_[(value << 8)];
		//}
		//else
		//{
			pix_value = (value << 8);
		//}
		int bin_start_point = top_remainder_y_ + left_remainder_x_;
		bin_start_point += bin_size_y_ * output_image_width_ * y_bin;
		bin_start_point += bin_size_x_ * x_bin;
		for (int k = 0; k < bin_size_y_; k++)
		{	// for each line in each bin
			int write_start_point = bin_start_point + (k * output_image_width_);
			for (int l = 0; l < bin_size_x_; l++)
			{	// for each space in each line
				int write_point = write_start_point + l;
				image[write_point*output_image_depth_] = (unsigned char)pix_value;
				image[(write_point*output_image_depth_) + 1] = (pix_value >> 8);
			}
		}
	}
}

void ImageScaler::ApplyLUT(unsigned char* image)
{
	unsigned char lsb, msb;
	int value;
	for (int i = 0; i < output_image_width_*output_image_height_*output_image_depth_; i += output_image_depth_)
	{
		lsb = image[i];
		msb = image[i + 1];
		value = (msb << 8) + lsb;
		value = lut_[value];
		lsb = (unsigned char)value;
		msb = value >> 8;
		image[i] = lsb;
		image[i + 1] = msb;
	}
}

void ImageScaler::ApplyLUTWFC(unsigned char* image)
{
	unsigned char lsb, msb, wfclsb, wfcmsb;
	int value, wfcvalue;
	for (int i = 0; i < output_image_width_*output_image_height_*output_image_depth_; i += output_image_depth_)
	{
		lsb = image[i];
		wfclsb = wavefront_correction_hack_[i];
		msb = image[i + 1];
		wfcmsb = wavefront_correction_hack_[i + 1];
		value = (msb << 8) + lsb;
		wfcvalue = (wfcmsb << 8) + wfclsb;
		value = (value + wfcvalue) % 65536;
		value = lut_[value];
		lsb = (unsigned char)value;
		msb = value >> 8;
		image[i] = lsb;
		image[i + 1] = msb;
	}
}
