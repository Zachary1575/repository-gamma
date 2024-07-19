make:
	as -g --32 memos.s -o memos.o
	ld -g -T memlayout.ld --oformat=binary memos.o -o memos
	ld -g -T memlayout.ld memos.o -o memos_ELF
	dd if=memos of=bootloader bs=512 count=1

clean:
	rm -f memos.o
	rm -f memos
	rm -f bootloader
	rm -f memos_ELF


