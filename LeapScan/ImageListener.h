#pragma once
#include "stdafx.h"

using namespace Leap;

class ImageListener : public Listener {
public:
	ImageListener(
		sf::Uint8* rawLeftPixels, sf::Uint8* rawRightPixels,
		sf::Uint8* distortionLeftPixels, sf::Uint8* distortionRightPixels,
		sf::Uint8* fixedLeftPixels, sf::Uint8* fixedRightPixels);

	virtual void onInit(const Controller&);
	virtual void onConnect(const Controller&);
	virtual void onDisconnect(const Controller&);
	virtual void onExit(const Controller&);
	virtual void onImages(const Controller&);
	virtual void onDeviceChange(const Controller&);
	virtual void onServiceConnect(const Controller&);
	virtual void onServiceDisconnect(const Controller&);

private:
	sf::Uint8* rawLeftPixels;
	sf::Uint8* rawRightPixels;
	sf::Uint8* distortionLeftPixels;
	sf::Uint8* distortionRightPixels;
	sf::Uint8* fixedLeftPixels;
	sf::Uint8* fixedRightPixels;
};