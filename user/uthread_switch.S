	.text

	/*
         * save the old thread's registers,
         * restore the new thread's registers.
         */

	.globl thread_switch
thread_switch:
	/* YOUR CODE HERE */
	sd x1, 0x00(a0)
	sd x2, 0x08(a0)
	sd x8, 0x10(a0)
	sd x9, 0x18(a0)
	sd x18, 0x20(a0)
	sd x19, 0x28(a0)
	sd x20, 0x30(a0)
	sd x21, 0x38(a0)
	sd x22, 0x40(a0)
	sd x23, 0x48(a0)
	sd x24, 0x50(a0)
	sd x25, 0x58(a0)
	sd x26, 0x60(a0)
	sd x27, 0x68(a0)

	ld x1, 0x00(a1)
	ld x2, 0x08(a1)
	ld x8, 0x10(a1)
	ld x9, 0x18(a1)
	ld x18, 0x20(a1)
	ld x19, 0x28(a1)
	ld x20, 0x30(a1)
	ld x21, 0x38(a1)
	ld x22, 0x40(a1)
	ld x23, 0x48(a1)
	ld x24, 0x50(a1)
	ld x25, 0x58(a1)
	ld x26, 0x60(a1)
	ld x27, 0x68(a1)
	ret    /* return to ra */
