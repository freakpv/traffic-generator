
fast:
	cd ./build && \
		touch version_info.cxx 2> /dev/null && \
		ninja --verbose

all:
	cd ./build && \
		touch version_info.cxx 2> /dev/null && \
		export TGN_CHECK_3RD_PARTY=1 && \
		ninja --verbose

clean:
	cd ./build && \
		ninja clean

configure:
	mkdir -p ./build && \
		cd ./build && \
		cmake --debug-output -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja ../

.PHONY: fast all clean configure
