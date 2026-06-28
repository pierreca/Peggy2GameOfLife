#ifndef __STEPCOUNTER_H__
#define __STEPCOUNTER_H__

class StepCounter 
{
	public:
		StepCounter();
		void ResetCurrentCount();
		void IncrementCount();
		char* GetCurrentCountString();
		char* GetMaxCountString();
		
	private:
		unsigned int currentCount;
		unsigned int maxCount;
		// One buffer per getter, so the conversions never touch the heap and the
		// two returned pointers never alias. 12 chars covers the widest unsigned
		// int on either platform (AVR 16-bit -> 5 digits, host 32-bit -> 10) plus
		// the null terminator.
		char currentCountString[12];
		char maxCountString[12];
		char *counterToString(unsigned int counter, char *buffer);
};

#endif
