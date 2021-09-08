#define FRAC (1<<14)

int int_float_sub(int i, int f);
int int_float_mul(int i, int f);
int float_int_add(int f, int i);
int float_float_mul(int f1, int f2);
int float_float_div(int f1, int f2);
int float_float_add(int f1, int f2);
int float_float_sub(int f1, int f2);
int float_int_div(int f, int i);
/*



int int_float_sub(int i, int f){
	int tmp = (i*FRAC - f);
	return tmp;
}

int int_float_mul(int i, int f){
	int tmp = i*f;
	return tmp;
}

int float_int_add(int f, int i){
	int tmp = (f+i*FRAC);
	return tmp;
}

int float_float_mul(int f1, int f2){
	int64_t tmp = f1;
	tmp = tmp*f2/FRAC;
	return (int)tmp;
}

int float_float_div(int f1, int f2){
	int64_t tmp = f1;
	tmp = tmp*FRAC/f2;
	return (int)tmp;
}

int float_float_add(int f1, int f2){
	int tmp = f1 + f2;
	return tmp;
}

int float_float_sub(int f1, int f2){
	int tmp = f1 - f2;
	return tmp;
}

int float_int_div(int f, int i){
	int tmp = f/i;
	return tmp;
}
*/
