#include "stdafx.h"
#include "ImageListener.h"



#include "opencv2\calib3d\calib3d.hpp"
#include "opencv2\core\core.hpp"
#include "opencv2\imgcodecs.hpp"
#include "opencv2\highgui\highgui.hpp"
#include "opencv2\imgproc\imgproc.hpp"

using namespace Leap;
using namespace cv;

int rawWidth = 640;
int rawHeight = 240;

int distortionWidth = 64;
int distortionHeight = 64;

int fixedWidth = 640;
int fixedHeight = 640;

int depthWidth = 640;
int depthHeight = 640;

int main() {
	// Create pixel array, texture and sprite for the RAW IMAGES
	sf::Uint8* rawLeftPixels = new sf::Uint8[rawWidth * rawHeight * 4]; // * 4 because pixels have 4 components (RGBA)
	sf::Texture rawLeftTexture;
	rawLeftTexture.create(rawWidth, rawHeight);
	rawLeftTexture.update(rawLeftPixels);
	sf::Sprite rawLeftSprite;
	rawLeftSprite.setTexture(rawLeftTexture);
	rawLeftSprite.setScale(0.5, 1);

	sf::Uint8* rawRightPixels = new sf::Uint8[rawWidth * rawHeight * 4];
	sf::Texture rawRightTexture;
	rawRightTexture.create(rawWidth, rawHeight);
	rawRightTexture.update(rawRightPixels);
	sf::Sprite rawRightSprite;
	rawRightSprite.setTexture(rawRightTexture);
	rawRightSprite.setScale(0.5, 1);
	rawRightSprite.setPosition(rawWidth * 0.5, 0);

	// Create pixel array, texture and sprite for the DISTORION MAP
	sf::Uint8* distortionLeftPixels = new sf::Uint8[distortionWidth * distortionHeight * 4];
	sf::Texture distortionLeftTexture;
	distortionLeftTexture.create(distortionWidth, distortionHeight);
	distortionLeftTexture.update(distortionLeftPixels);
	sf::Sprite distortionLeftSprite;
	distortionLeftSprite.setTexture(distortionLeftTexture);
	distortionLeftSprite.setPosition(0, rawHeight);

	sf::Uint8* distortionRightPixels = new sf::Uint8[distortionWidth * distortionHeight * 4];
	sf::Texture distortionRightTexture;
	distortionRightTexture.create(distortionWidth, distortionHeight);
	distortionRightTexture.update(distortionRightPixels);
	sf::Sprite distortionRightSprite;
	distortionRightSprite.setTexture(distortionRightTexture);
	distortionRightSprite.setPosition(rawWidth * 0.5, rawHeight);

	// Create pixel array, texture and sprite for the FIXED IMAGES
	sf::Uint8* fixedLeftPixels = new sf::Uint8[fixedWidth * fixedHeight * 4];
	sf::Texture fixedLeftTexture;
	fixedLeftTexture.create(fixedWidth, fixedHeight);
	fixedLeftTexture.update(fixedLeftPixels);
	sf::Sprite fixedLeftSprite;
	fixedLeftSprite.setTexture(fixedLeftTexture);
	fixedLeftSprite.setScale(0.5, 0.5);
	fixedLeftSprite.setPosition(0, rawHeight + distortionHeight);

	sf::Uint8* fixedRightPixels = new sf::Uint8[fixedWidth * fixedHeight * 4];
	sf::Texture fixedRightTexture;
	fixedRightTexture.create(fixedWidth, fixedHeight);
	fixedRightTexture.update(fixedRightPixels);
	sf::Sprite fixedRightSprite;
	fixedRightSprite.setTexture(fixedRightTexture);
	fixedRightSprite.setScale(0.5, 0.5);
	fixedRightSprite.setPosition(rawWidth * 0.5, rawHeight + distortionHeight);

	// Create pixel array, texture and sprite for the DEPTH IMAGE
	sf::Texture depthTexture;
	depthTexture.create(fixedWidth, fixedHeight);
	sf::Sprite depthSprite;
	depthSprite.setTexture(depthTexture);
	depthSprite.setPosition(rawWidth, 0);

	// Set up the Leap
	ImageListener listener(
		rawLeftPixels, rawRightPixels,
		distortionLeftPixels, distortionRightPixels,
		fixedLeftPixels, fixedRightPixels);
	Controller controller;
	controller.setPolicy(Leap::Controller::POLICY_IMAGES);
	controller.addListener(listener);

	// create a window for debug vis
	sf::RenderWindow window(sf::VideoMode(fixedWidth * 2, fixedHeight), "Leap Scan");

	// Set up some OpenCV properties to tweak at runtime
	int SADWindowSize = 5;
	int numberOfDisparities = 12;
	int preFilterCap = 4;
	int minDisparity = -64;
	int uniquenessRatio = 1;
	int speckleWindowSize = 150;
	int speckleRange = 2;
	int disp12MaxDiff = 10;
	int fullDP = false;
	int P1 = 600;
	int P2 = 2400;

	int simple = 1;
	int slow = 2;
	int method = simple;

	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				window.close();
			}
			else if (event.type == sf::Event::KeyPressed) {
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
					numberOfDisparities = std::max(1, numberOfDisparities - 1);
				}
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
					numberOfDisparities = std::min(30, numberOfDisparities + 1);
				}
				else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
					SADWindowSize = std::max(5, SADWindowSize - 2);
				}
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
					SADWindowSize = std::min(101, SADWindowSize + 2);
				}
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) {
					method = simple;
				}
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) {
					method = slow;
				}

				std::cout << "Params:"
					<< " numberOfDisparities=" << numberOfDisparities
					<< " SADWindowSize=" << SADWindowSize
					<< " method=" << (method==simple ? "simple" : method==slow ? "slow" : "unknown")
					<< std::endl;
			}
		}

		// Update textures with data from the leap (potentially need to add a mutex if tearing starts to happen)
		rawLeftTexture.update(rawLeftPixels);
		rawRightTexture.update(rawRightPixels);
		distortionLeftTexture.update(distortionLeftPixels);
		distortionRightTexture.update(distortionRightPixels);
		fixedLeftTexture.update(fixedLeftPixels);
		fixedRightTexture.update(fixedRightPixels);

		// Run some OpenCV magic to attempt a 3D reconstruction
		Mat imgLeft(fixedHeight, fixedWidth, CV_8UC4, fixedLeftPixels);
		Mat imgRight(fixedHeight, fixedWidth, CV_8UC4, fixedRightPixels);
		Mat left, right, output;
		Mat imgDisparity16S = Mat(imgLeft.rows, imgLeft.cols, CV_16S);
		Mat imgDisparity8U = Mat(imgLeft.rows, imgLeft.cols, CV_8U);
		cvtColor(imgLeft, left, COLOR_RGBA2GRAY);
		cvtColor(imgRight, right, COLOR_RGBA2GRAY);

		if (method == simple) {
			// Fast but meh method from OpenCV
			Ptr<StereoBM> sbm = StereoBM::create(numberOfDisparities * 16, SADWindowSize);
			sbm->compute(left, right, imgDisparity16S);
		}
		else if (method == slow) {
			// Slower but "more accurate" method
			Ptr<StereoSGBM> sgbm = StereoSGBM::create(
				minDisparity,
				numberOfDisparities * 16,
				SADWindowSize,
				P1,
				P2,
				disp12MaxDiff,
				preFilterCap,
				uniquenessRatio,
				speckleWindowSize,
				speckleRange,
				StereoSGBM::MODE_SGBM);
			sgbm->compute(left, right, imgDisparity16S);
		}

		// TODO - There is a GPU method in OpenCV I could try to get better results and now be slow

		// Convert the OpenCV output to a format SFML likes
		double minVal; double maxVal;
		minMaxLoc(imgDisparity16S, &minVal, &maxVal);
		imgDisparity16S.convertTo(imgDisparity8U, CV_8U, 255 / (maxVal - minVal));
		cvtColor(imgDisparity8U, output, COLOR_GRAY2BGRA);
		// Update the depth buffer texture
		depthTexture.update(output.ptr());

		// Redraw the window
		window.clear(sf::Color(0x33, 0x33, 0x33));
		window.draw(rawLeftSprite);
		window.draw(rawRightSprite);
		window.draw(distortionLeftSprite);
		window.draw(distortionRightSprite);
		window.draw(fixedLeftSprite);
		window.draw(fixedRightSprite);
		window.draw(depthSprite);
		window.display();
	}

	// Make sure to clean up the leap on close
	controller.removeListener(listener);

	return 0;
}
