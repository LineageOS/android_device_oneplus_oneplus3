all:
	@echo "invoking omxaudio make"
	$(MAKE) -C adec-mp3
	$(MAKE) -C adec-aac
	$(MAKE) -C aenc-aac

install:
	$(MAKE) -C adec-mp3 install
	$(MAKE) -C adec-aac install
	$(MAKE) -C aenc-aac install
