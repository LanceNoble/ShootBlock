#include <stdio.h>
#include <stdlib.h>

struct Message {
	unsigned char len;
	char buf[0xff];
};

struct Message foo() {
	struct Message bar;
	bar.len = 3;
	bar.buf[0] = 'p';
	bar.buf[1] = 'o';
	bar.buf[2] = 'o';
	return bar;
}

void test(struct Message thing) {
	printf("%s", thing.buf);
}

char* banana() {
	char nope[3];
	nope[0] = 'D';
	nope[1] = 'A';
	nope[2] = 'Y';
	return nope;
}

int main() {
	/*
	char test[3];
	test[0] = -128;
	test[1] = 0;
	test[2] = 127;

	unsigned char* test1 = test;

	unsigned char hi = test1[0];
	unsigned char hi1 = test1[1];
	unsigned char hi2 = test1[2];
	*/

	//printf("%s", "test");
	struct Message hi = foo();
	//printf("%u", hi->len);
	char* bye = banana();
	test(hi);
	char eeee = bye[1];
	return 1;
}