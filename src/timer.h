#pragma once

class Timer {
    float timeSMax;
    float timeSCurrent;

public:
    Timer(float setTimeSMax, float setTimeSCurrent=0.0f);
    
    void countUp(float dT);
	void countDown(float dT);
	void resetToZero();
	void resetToMax();
	void setTo(float timeS);
	bool timeSIsZero();
	bool timeSIsGreaterThanOrEqualTo(float timeSCheck);
};