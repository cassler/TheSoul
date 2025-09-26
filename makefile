build:
	em++ -O3 -pthread --closure 1 -lembind -o immolate.js include/immolate.cpp -s EXPORT_NAME="'Immolate'" -s USE_PTHREADS=1 -s PROXY_TO_PTHREAD=1

.PHONY: build