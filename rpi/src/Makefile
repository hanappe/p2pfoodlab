
prefix = ..
programs = ${prefix}/bin/p2pfoodlab-daemon ${prefix}/bin/grab-image ${prefix}/bin/config-get ${prefix}/bin/arduino

all: ${programs}

${prefix}/bin/p2pfoodlab-daemon: daemon.c logMessage.c logMessage.h
	gcc -g -Wall -O0 -std=c99 daemon.c logMessage.c -o $@

${prefix}/bin/grab-image: grab-image.c json.c json.h logMessage.c logMessage.h 
	gcc -g -Wall -O0 -std=c99 grab-image.c json.c logMessage.c -lm -ljpeg -o $@

${prefix}/bin/config-get: config-get.c json.c json.h logMessage.c logMessage.h
	gcc -g -Wall -O0 -std=c99 config-get.c json.c logMessage.c -lm -o $@

${prefix}/bin/arduino: arduino.c json.c json.h logMessage.c logMessage.h
	gcc -g -Wall -O0 -std=c99 arduino.c json.c logMessage.c -lm -o $@

clean:
	rm -f ${programs}
