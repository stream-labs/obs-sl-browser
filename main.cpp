#include "SlBrowser.h"

int main(int argc, char *argv[])
{
	AllocConsole();
	freopen("conin$", "r", stdin);
	freopen("conout$", "w", stdout);
	freopen("conout$", "w", stderr);
	printf("Streamlabs Browser:\n");

	SlBrowser::instance().run(argc, argv);
	return true;
}
