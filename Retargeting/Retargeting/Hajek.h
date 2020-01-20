#pragma once
#include "AnnealingSchedule.h"
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include <stdio.h>
#include <sstream>

class Hajek :
	public AnnealingSchedule
{
public:
	Hajek();
	Hajek(double T_0, double c);
	double getTemperature(int step);
	std::string getName();
	std::string getFunction();
private:
	double T_0;
	double c;
};
