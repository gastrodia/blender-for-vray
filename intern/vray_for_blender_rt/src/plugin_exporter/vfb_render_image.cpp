/*
 * Copyright (c) 2015, Chaos Software Ltd
 *
 * V-Ray For Blender
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "vfb_render_image.h"

#include <cstring>
#include <algorithm>

using namespace VRayForBlender;


void RenderImage::free()
{
	FreePtrArr(pixels);
}


void RenderImage::flip()
{
	if (pixels && w && h) {
		const int _half_h = h / 2;
		const int half_h = h % 2 ? _half_h : _half_h - 1;

		const int row_items = w * channels;
		const int row_bytes = row_items * sizeof(float);

		float *buf = new float[row_items];

		for (int i = 0; i < half_h; ++i) {
			float *to_row   = pixels + (i       * row_items);
			float *from_row = pixels + ((h-i-1) * row_items);

			memcpy(buf,      to_row,   row_bytes);
			memcpy(to_row,   from_row, row_bytes);
			memcpy(from_row, buf,      row_bytes);
		}

		FreePtr(buf);
	}
}


void RenderImage::resetAlpha()
{
	if (channels == 4 && pixels && w && h) {
		const int pixelCount = w * h;
		for (int p = 0; p < pixelCount; ++p) {
			float *pixel = pixels + (p * channels);
			pixel[3] = 1.0f;
		}
	}
}


void RenderImage::clamp(float max, float val)
{
	if (pixels && w && h) {
		const int pixelCount = w * h;
		for (int p = 0; p < pixelCount; ++p) {
			float *pixel = pixels + (p * channels);
			switch (channels) {
			case 3: pixel[2] = pixel[2] > max ? val : pixel[2];
			case 2: pixel[1] = pixel[1] > max ? val : pixel[1];
			case 1: pixel[0] = pixel[0] > max ? val : pixel[0];
			}
		}
	}
}
