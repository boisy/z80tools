z80lint:	z80lint.c

run: z80lint
	./z80lint test.asm
	
clean:
	rm z80lint