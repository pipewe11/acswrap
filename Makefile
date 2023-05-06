
CFLAGS = -O2
#CFLAGS = -g

acswrap: acswrap.cpp
	clang++-12 $(CFLAGS) -o $@ $^ \
		/usr/lib/x86_64-linux-gnu/libboost_program_options.a \
		/usr/lib/x86_64-linux-gnu/libboost_filesystem.a \
		-lpthread

.PHONY:
upload-temp:
	scp acswrap oc:~/assetto/acswrap/acswrap.temp
	scp acswrap oc2:~/assetto/acswrap/acswrap.temp

.PHONY:
upload-rc:
	scp acswrap oc:~/assetto/acswrap/acswrap.rc
	scp acswrap oc2:~/assetto/acswrap/acswrap.rc

.PHONY:
clean:
	rm -f acswrap

