
prefix = ..
programs = ${prefix}/bin/p2pfoodlab-daemon ${prefix}/bin/grab-image ${prefix}/bin/config-get ${prefix}/bin/arduino

all: ${programs}

${prefix}/bin/p2pfoodlab-daemon: daemon.c logMessage.c logMessage.h
	gcc -g -Wall -O0 -std=c99 daemon.c logMessage.c -o $@

${prefix}/bin/grab-image: grab-image.c json.c json.h logMessage.c logMessage.h 
	gcc -g -Wall -O0 -std=c99 grab-image.c json.c logMessage.c -lm -ljpeg -o $@

${prefix}/bin/config-get: config-get.c json.c json.h logMessage.c logMessage.h
	gcc -g -Wall -O0 -std=c99 config-get.c json.c logMessage.c -lm -o $@

${prefix}/bin/arduino: arduino-cli.c json.c json.h logMessage.c logMessage.h
	gcc -g -Wall -O0 -std=c99 arduino-cli.c json.c logMessage.c -lm -o $@

${prefix}/bin/p2pfoodlab-update: p2pfoodlab-update.c json.c json.h log_message.c log_message.h arduino.c arduino.h camera.c camera.h opensensordata.c opensensordata.h config.c config.h event.c event.h sensorbox.c sensorbox.h network.c network.h clock.c clock.h
	gcc -g -Wall -O0 -std=c99 -DIO_READ -DIO_MMAP -DIO_USERPTR p2pfoodlab-update.c json.c log_message.c arduino.c camera.c opensensordata.c config.c event.c sensorbox.c network.c clock.c -ljpeg -lcurl -lm -o $@

rtc-arduino: rtc-arduino.c 
	gcc -g -Wall -O0 -std=c99 rtc-arduino.c -o $@

clean:
	rm -f ${programs}
