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
		char *counterToString(unsigned int counter);
};

#endif
