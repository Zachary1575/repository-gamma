make:
	as --32 memos-2.s -o memos-2.o
	gcc -m32 -ffreestanding -fno-stack-protector -I./ -c kernel.c -o kernel.o
	ld -nostdlib -T memos-2.ld memos-2.o kernel.o -o memos-2
	cp ./memos-2 /mnt/C/boot/
	rm -f memos-2.o
	rm -f kernel.o
	rm -f memos-2
 
