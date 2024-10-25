build:
	mkdir -p bin
	echo Using: ${GCC_COMPILER}
	${GCC_COMPILER}-g++ main.cpp Lora.cpp -o ./bin/sx1278

deploy:
	adb push ./bin/sx1278 /oem/sx1278
