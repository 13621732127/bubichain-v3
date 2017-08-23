.PHONY:all
all:
	cd build/linux; \
	cmake \
	    -DSVNVERSION=$(bubi_version) \
	    -DCMAKE_BUILD_TYPE=Debug \
	    -DCMAKE_C_FLAGS="-O2 -Wall" \
	    -DCMAKE_CXX_FLAGS="-O2 -Wall" \
	    -DCMAKE_VERBOSE_MAKEFILE=ON \
	    ../../src; \
	make

.PHONY:clean_all clean clean_build clean_3rd
clean_all:clean clean_build clean_3rd

clean:
	rm -rf bin && rm -rf lib

clean_3rd:
	cd src/3rd && make clean_3rd && cd ../../

clean_build:
	rm -rf build/linux/*

.PHONY:install uninstall
install:
	cd build/linux && make install && make soft_link -f MakeSupplement

uninstall:
	cd build/linux && make uninstall -f MakeSupplement

