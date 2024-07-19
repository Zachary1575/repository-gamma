OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
	. = 0x7C00;	

	.text : 
	{
		*(.text)	
	}

	.data : 
	{
		*(.data)	
	}

	. = 0x7C000;

	.bss : 
	{
		*(.bss)
	}
}
