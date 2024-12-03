#pragma once

union PackedFloat {
	unsigned long pack;
	unsigned char raw[4];
};

unsigned long pack_float(float den);
float unpack_float(unsigned long bin);