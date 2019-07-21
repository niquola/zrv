.PHONY: test deploy

.EXPORT_ALL_VARIABLES:

build:
	gcc -pthread zrv.c -o zrv

init:
	docker build -t zrv .

env:
	docker-compose up -d
	docker exec -it zrv bash
