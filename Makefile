make:
	gcc mt-sync.c -o mt-sync && ./mt-sync

clean:
	rm -f mt-sync

run:
	./mt-sync

.PHONY: main clean run