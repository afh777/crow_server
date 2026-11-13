#include "functions.h"
#include <string>

struct Lights {
	int index;
	int red;
	int green;
	int blue;
	int brightness;
};

Lights currentColor = {0, 0, 0, 0, 128};

void turnLightsOff() {
	//neopixel stuff
}

void turnLightsOn() {
	//read current color to turn all pixels on
}

void turnLightsChasing() {
	//read current color to turn chasing lights on
}

void setColor(std::string hex) {
	//convert hex to RGB
	//store in currentColor.red, green, blue
}
