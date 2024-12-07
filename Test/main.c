#include <stdio.h>

union Test {
	unsigned long a;
	unsigned char b[4];
};

void main() {
	printf("%f", 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067);
	union Test a;
	a.a = 255;
	//a.b[0] = 255;
	//a.b[1] = 0;
	//a.b[2] = 0;
	//a.b[3] = 0;
	a.a = 1;//(64 << 24) | (63 << 16) | (62 << 8) | (61);

	union Test b;
	b.b[0] = a.b[0];
	b.b[1] = a.b[1];
	b.b[2] = a.b[2];
	b.b[3] = a.b[3];

	b.b[0] = 0			 ;
		b.b[1] = 0		 ;
		b.b[2] = 0		 ;
		b.b[3] = 1		 ;




}