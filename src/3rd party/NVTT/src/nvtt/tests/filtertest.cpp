
#include <nvimage/Filter.h>
#include "../tools/cmdline.h"

#include <math.h>

using namespace nv;

int main(void)
{
//	MyAssertHandler assertHandler;
	MyMessageHandler messageHandler;

	BoxFilter box1(0.5);
	Kernel1 k1(box1, 2);
	k1.debugPrint(); nvDebug("\n");

	BoxFilter box2(1);
	Kernel1 k2(box2, 2);
	k2.debugPrint(); nvDebug("\n");

	BoxFilter boxr3(1);
	Kernel1 k3(boxr3, 2);
	k3.debugPrint(); nvDebug("\n");

	KaiserFilter kai4(5);
	kai4.setParameters(4, 2);
	Kernel1 k4(kai4, 2);
	k4.debugPrint(); nvDebug("\n");

/*	Kernel1 k3(3);
	Kernel1 k4(9);
	Kernel1 k5(10);

//	k3.initFilter(Filter::Box);
//	k4.initFilter(Filter::Box);
//	k5.initFilter(Filter::Box);

//	nvDebug("Box Filter:\n");
//	k3.debugPrint(); nvDebug("\n");
//	k4.debugPrint(); nvDebug("\n");
//	k5.debugPrint(); nvDebug("\n");

	k3.initSinc(0.75);
	k4.initSinc(0.75);
	k5.initSinc(0.75);

	nvDebug("Sinc Filter:\n");
	k3.debugPrint(); nvDebug("\n");
	k4.debugPrint(); nvDebug("\n");
	k5.debugPrint(); nvDebug("\n");
	
	k3.initKaiser(4, 1, 100);
	k4.initKaiser(4, 1, 100);
	k5.initKaiser(4, 1, 100);

	nvDebug("Kaiser Filter:\n");
	k3.debugPrint(); nvDebug("\n");
	k4.debugPrint(); nvDebug("\n");
	k5.debugPrint(); nvDebug("\n");

	k3.initKaiser(4, 1, 10);
	k4.initKaiser(4, 1, 10);
	k5.initKaiser(4, 1, 10);

	nvDebug("Kaiser Filter 2:\n");
	k3.debugPrint(); nvDebug("\n");
	k4.debugPrint(); nvDebug("\n");
	k5.debugPrint(); nvDebug("\n");
*/	
	int l_start = 4;
	int l_end = 2;
	
	BoxFilter filter;
	PolyphaseKernel kp(kai4, l_start, l_end);
	
	kp.debugPrint();
	
	return 0;
}
