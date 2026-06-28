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
	// Size for the widest decimal string itoa can produce: at most 3 digits
	// per byte (the slack also absorbs the '-' itoa emits for base-10 values
	// above INT_MAX), plus the null terminator.
	char *result = (char *)malloc(sizeof(unsigned int) * 3 + 1);
	itoa(counter, result, 10);
	
	return result;
}
