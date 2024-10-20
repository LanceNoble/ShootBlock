#define MEM_DEBUG_MODE

int main() {

	mem_track_create();
	char* a = malloc(sizeof(char) * 8);
	int* b = malloc(sizeof(int) * 2);
	double* c = malloc(sizeof(double));
	mem_track_show();
	free(a);
	free(b);
	free(c);
	mem_track_show();
	mem_track_destroy();

	wsa_create();
	wsa_create();
	wsa_create();

	wsa_destroy();
	wsa_destroy();
	wsa_destroy();

	return 0;
}