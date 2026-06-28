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
	return counterToString(this->currentCount, this->currentCountString);
}

char* StepCounter::GetMaxCountString()
{
	return counterToString(this->maxCount, this->maxCountString);
}

char *StepCounter::counterToString(unsigned int counter, char *buffer)
{
	// Fills a caller-owned member buffer instead of allocating a new string each
	// call, so the steady-state loop produces zero heap churn (issue #1).
	// utoa (not itoa): counter is unsigned and runs past INT_MAX on a board left
	// running for hours, which signed itoa would render as a negative number.
	utoa(counter, buffer, 10);
	return buffer;
}
