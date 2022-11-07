
CFLAGS = -O2
#CFLAGS = -g

acswrap: acswrap.cpp
	clang++-12 $(CFLAGS) -o $@ $^ \
		/usr/lib/x86_64-linux-gnu/libboost_program_options.a \
		/usr/lib/x86_64-linux-gnu/libboost_filesystem.a \
		-lpthread

.PHONY:
upload:
	scp acswrap indigo:~/assetto/acswrap/acswrap.temp

.PHONY:
clean:
	rm -f acswrap

