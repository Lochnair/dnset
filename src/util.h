static inline char * strrev(char * str)
{
	const int l = strlen(str);
	char* rs = kmalloc(l + 1, GFP_KERNEL);
	int i;

	for (i = 0; i < l; i++) {
		rs[i] = str[l - 1 - i];
	}

	rs[l] = '\0';

	return rs;
}
