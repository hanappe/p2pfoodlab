
prefix = ..
programs = ${prefix}/bin/p2pfoodlab-daemon ${prefix}/bin/sensorbox

all: ${programs}

${prefix}/bin/p2pfoodlab-daemon: daemon.c log_message.c log_message.h
	gcc -g -Wall -O0 -std=c99 daemon.c log_message.c -o $@

${prefix}/bin/sensorbox: main.c json.c json.h log_message.c log_message.h arduino.c arduino.h camera.c camera.h opensensordata.c opensensordata.h config.c config.h event.c event.h sensorbox.c sensorbox.h system.c system.h network.c network.h clock.c clock.h
	gcc -g -Wall -O0 -std=c99 -DIO_READ -DIO_MMAP -DIO_USERPTR main.c json.c log_message.c arduino.c camera.c opensensordata.c config.c event.c sensorbox.c system.c network.c clock.c -ljpeg -lcurl -lm -o $@

#rtc-arduino: rtc-arduino.c 
#	gcc -g -Wall -O0 -std=c99 rtc-arduino.c -o $@

install:
	cp ${prefix}/bin/p2pfoodlab-daemon /var/p2pfoodlab/bin/
	cp ${prefix}/bin/sensorbox /var/p2pfoodlab/bin/

clean:
	rm -f ${programs}
