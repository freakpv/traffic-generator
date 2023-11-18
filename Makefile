
all:
	cd ./build && touch version_info.cpp 2> /dev/null && ninja --verbose

clean:
	cd ./build && ninja clean

configure:
	mkdir -p ./build && \
		cd ./build && \
		cmake --debug-output -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja ../

.PHONY: all clean configure
