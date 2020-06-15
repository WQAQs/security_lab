#include <stdlib.h>
#include <string.h>


int rc4_encrypt(char *P, char *C, char *K, int fileLen) {
	int S[256], R[256];

	int i, j;                    //初始化S
	for (i = 0; i < 256; i++)
		S[i] = i;

	int len = 128;           //用种子K填充R
	for (i = 0; i < 256; i++) {
		j = i % len;
		R[i] = K[j];
	}

	j = 0;                        //用R随机化S
	for (i = 0; i < 256; i++) {
		j = (j + S[i] + R[i]) % 256;
		int c = S[i];
		S[i] = S[j];
		S[j] = c;
	}


	int *keystream;
	keystream = (int *)malloc(sizeof(int)*fileLen);


	i = 0;
	j = 0;
	len = 0;
	while (len < fileLen) {
		j = (j + S[i]) % 256;
		int c = S[i];
		S[i] = S[j];
		S[j] = c;
		int h = (S[i] + S[j]) % 256;
		keystream[len] = S[h];
		C[len] = (char)(keystream[len] ^ P[len]);
		i = (i + 1) % 256;
		len++;
	}

	return 1;
}


