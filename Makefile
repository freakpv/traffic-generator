
all:
	cd ./build && ninja --verbose

clean:
	cd ./build && ninja clean

configure:
	mkdir -p ./build && cd ./build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja ../

.PHONY: all clean configure
