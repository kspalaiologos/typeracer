
typeracer: typeracer.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

install:
	install -v -m 755 typeracer /usr/bin
	install -v -m 644 typeracer-dict /usr/share/typeracer-dict

uninstall:
	rm -fv /usr/bin/typeracer
	rm -rfv /usr/share/typeracer-dict

clean:
	rm -f typeracer
