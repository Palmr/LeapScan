#include "stdafx.h"
#include "ImageListener.h"

ImageListener::ImageListener(
	sf::Uint8* rawLeftPixels, sf::Uint8* rawRightPixels,
	sf::Uint8* distortionLeftPixels, sf::Uint8* distortionRightPixels,
	sf::Uint8* fixedLeftPixels, sf::Uint8* fixedRightPixels)
{
	this->rawLeftPixels = rawLeftPixels;
	this->rawRightPixels = rawRightPixels;
	this->distortionLeftPixels = distortionLeftPixels;
	this->distortionRightPixels = distortionRightPixels;
	this->fixedLeftPixels = fixedLeftPixels;
	this->fixedRightPixels = fixedRightPixels;
}

void ImageListener::onInit(const Controller& controller) {
	std::cout << "Initialized" << std::endl;
}

void ImageListener::onConnect(const Controller& controller) {
	std::cout << "Connected" << std::endl;
}

void ImageListener::onDisconnect(const Controller& controller) {
	// Note: not dispatched when running in a debugger.
	std::cout << "Disconnected" << std::endl;
}

void ImageListener::onExit(const Controller& controller) {
	std::cout << "Exited" << std::endl;
}

void ImageListener::onImages(const Controller& controller) {
	sf::Uint8 *rawFrameBuffer, *distortionFrameBuffer, *fixedFrameBuffer;
	ImageList images = controller.images();
	for (int i = 0; i < 2; i++) {
		if (i == 0) {
			rawFrameBuffer = this->rawLeftPixels;
			distortionFrameBuffer = this->distortionLeftPixels;
			fixedFrameBuffer = this->fixedLeftPixels;
		}
		else {
			rawFrameBuffer = this->rawRightPixels;
			distortionFrameBuffer = this->distortionRightPixels;
			fixedFrameBuffer = this->fixedRightPixels;
		}

		Image image = images[i];

		// Render the raw buffer to the raw frame buffer
		const unsigned char* imageBuffer = image.data();
		for (int x = 0; x < image.width(); x++) {
			for (int y = 0; y < image.height(); y++) {
				int position = ((y * image.width()) + x) * image.bytesPerPixel();
				rawFrameBuffer[(position * 4) + 0] = imageBuffer[position];
				rawFrameBuffer[(position * 4) + 1] = imageBuffer[position];
				rawFrameBuffer[(position * 4) + 2] = imageBuffer[position];
				rawFrameBuffer[(position * 4) + 3] = 0xff;
			}
		}
		
		// Render the distortion buffer to the distortion frame buffer (this isn't suitable for shader use, just debug vis)
		const float* distBuffer = image.distortion();
		int cursor = 0;
		int dw = image.distortionWidth() / 2;
		int dh = image.distortionHeight();
		for (int y = 0; y < dh; y++) {
			for (int x = 0; x < dw; x++) {
				int position = ((y * dw) + x);

				float input1 = (distBuffer[cursor] + 0.6) / 2.3; //scale the input value to the range [0..1]
				float input2 = (distBuffer[cursor + 1] + 0.6) / 2.3; //scale the input value to the range [0..1]

				if (distBuffer[cursor] > 0.0 && distBuffer[cursor] < 1.0
					&& distBuffer[cursor + 1] > 0.0 && distBuffer[cursor + 1] < 1.0)
				{
					distortionFrameBuffer[(position * 4) + 0] = input1 * 255;
					distortionFrameBuffer[(position * 4) + 1] = input2 * 255;
					distortionFrameBuffer[(position * 4) + 2] = 0xff;
					distortionFrameBuffer[(position * 4) + 3] = 0xff;
				}
				else {
					distortionFrameBuffer[(position * 4) + 0] = 0xff;
					distortionFrameBuffer[(position * 4) + 1] = 0x00;
					distortionFrameBuffer[(position * 4) + 2] = 0x00;
					distortionFrameBuffer[(position * 4) + 3] = 0xff;
				}
				cursor += 2;
			}
		}

		// Render the undistorted image buffer to the fixed frame buffer (ideally do this in shaders for speed)
		int ow = 640;
		int oh = 640;
		unsigned char brightness[4] = { 0,0,0,255 }; //An array to hold the rgba color components
		for (int y = 0; y < oh; y++) {
			for (int x = 0; x < ow; x++) {
				int position = ((y * ow) + x);

				Vector input = Vector((float)x / ow, (float)y / oh, 0);
				input.x = (input.x - image.rayOffsetX()) / image.rayScaleX();
				input.y = (input.y - image.rayOffsetY()) / image.rayScaleY();

				Vector pixel = image.warp(input);

				if (pixel.x >= 0 && pixel.x < image.width() && pixel.y >= 0 && pixel.y < image.height()) {
					int data_index = floor(pixel.y) * image.width() + floor(pixel.x); //xy to buffer index
					brightness[0] = image.data()[data_index]; //Look up brightness value
					brightness[2] = brightness[1] = brightness[0]; //Greyscale
				}
				else {
					brightness[0] = 255; //Display invalid pixels as red
					brightness[2] = brightness[1] = 0;
				}

				fixedFrameBuffer[(position * 4) + 0] = brightness[0];
				fixedFrameBuffer[(position * 4) + 1] = brightness[1];
				fixedFrameBuffer[(position * 4) + 2] = brightness[2];
				fixedFrameBuffer[(position * 4) + 3] = brightness[3];
			}
		}
	}
}

void ImageListener::onDeviceChange(const Controller& controller) {
	std::cout << "Device Changed" << std::endl;
	const DeviceList devices = controller.devices();

	for (int i = 0; i < devices.count(); ++i) {
		std::cout << "id: " << devices[i].toString() << std::endl;
		std::cout << "  isStreaming: " << (devices[i].isStreaming() ? "true" : "false") << std::endl;
	}
}

void ImageListener::onServiceConnect(const Controller& controller) {
	std::cout << "Service Connected" << std::endl;
}

void ImageListener::onServiceDisconnect(const Controller& controller) {
	std::cout << "Service Disconnected" << std::endl;
}
