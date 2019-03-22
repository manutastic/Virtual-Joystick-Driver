chompdrv: chompdrv.c
	gcc -o chompdrv chompdrv.c -lusb-1.0

clean:
	 rm chompdrv
