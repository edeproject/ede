#include "../Sound.h"
#include <stdio.h>

int main()
{
	puts("playing...\n");
	SoundSystem ss;
	ss.init();
	ss.play("demo.ogg");
	ss.shutdown();
	puts("done\n");

	return 0;
}
