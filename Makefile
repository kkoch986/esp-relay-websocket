PIO=${HOME}/.local/bin/pio
BAUDRATE=115200
ENVSTR=
ifdef PIOENV
	ENVSTR="-e"${PIOENV}
endif

preclean:
	rm .pio/libdeps/${PIOENV}/OSC/SLIPEncodedBluetoothSerial.cpp || true

compile:
	${PIO} run ${ENVSTR}

upload:
	${PIO} run --target=upload ${ENVSTR}

upload-thingaddams:
	PIOENV="thingaddams" make preclean
	${PIO} run --target=upload -e thingaddams

logs: 
	${PIO} device monitor -b ${BAUDRATE} ${ENVSTR}

all: compile

.PHONY: compile upload logs preclean all upload-thingaddams
