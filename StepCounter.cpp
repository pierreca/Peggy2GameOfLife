#include <stdlib.h>
#include "StepCounter.h"

StepCounter::StepCounter()
{
	this->currentCount = 0;
	this->maxCount = 0;
}

void StepCounter::ResetCurrentCount()
{
	this->currentCount = 0;
}

void StepCounter::IncrementCount()
{
	this->currentCount++;
	if (this->currentCount > this->maxCount)
	{
		this->maxCount = this->currentCount;
	}
}

char* StepCounter::GetCurrentCountString()
{
	return counterToString(this->currentCount);
}

char* StepCounter::GetMaxCountString()
{
	return counterToString(this->maxCount);
}

char *StepCounter::counterToString(unsigned int counter)
{
	unsigned short length = 2; // at least 1 number + null terminator
	
	if (counter > 10) length++;
	if (counter > 100) length++;
	if (counter > 1000) length++;
	if (counter > 10000) length++;
	
	char *result = (char *)malloc(length);
	itoa(counter, result, 10);
	
	return result;
}
