#ifndef SWFAA_AUTO
#define SWFAA_AUTO

#include <WProgram.h>

class Autonomous {
public:
	Autonomous();
	~Autonomous();

	void TimerTick();

	void BlueChin();
	void RedChin();
	void BlueGoal();
	void RedGoal();
};

#endif