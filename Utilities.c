
#include "Track.h"

double	angle(int x, int y, int xc, int yc)
{
	double	tangle;

	x -= xc;
	y -= yc;
	tangle = M_PI - atan2((double)y,(double)-x);
	return tangle;
}


FILE	*openfile(char *s, char *a)
{
	FILE	*g;

	g = fopen(s, a);
	if (g == NULL){
		fprintf(MyStdOut, "Can't open file %s\n", s);
	}
	return g;
}

int	*allocate(int e)
{
	int *u;

	u = (int *) calloc(e, sizeof(int));	/* allocation of pointers that store the values of	*/
	if (u == NULL){
		fprintf(MyStdOut, "Can't allocate memory for pointers!");
		exit(-4);
	}
	return u;
}

int	GetChars(char *String)
{
	int	i = 0, c;

	while (1){
		c = fgetc(stdin);
		if (c == EOF)
			return c;

		if(c == '\n')
			break;

		String[i] = (char)c;
		i++;
	}
	sprintf(String+i,"%c", '\0');

	return i;
}

char	**string_alloc(char **p, int m, int n)
{
	int	i;

	p = (char **) calloc(m, sizeof(char *));
	if (p == NULL){
		fprintf(MyStdOut, "\n Can't allocate memory!");
		exit(-10);
	}
	for (i = 0; i < m; ++i){
		*(p + i) = (char *) calloc(n, sizeof(char));
		if (*(p + i) == NULL){
			fprintf(MyStdOut, "\n Can't allocate memory!");
			exit(-10);
		}
	}
	return p;
}

void	fill(char **a, int r, char **b)
{
	int	i;

	for (i = 0; i < r - 2; ++i)
		strcpy(*(a + i), *(b + i));
	for (i = r - 2; i < 3; ++i)
		strcpy(*(a + i), "");
}

int	get_comment(void)
{
	int	par = 0;

	while (par == 0){
		fprintf(MyStdOut, "Input number  of characters the individual files must match\nin order to be processed together (2 - 8):\n");
		scanf("%d", &par);
		if ((par < 2) || (par > 8)){
			fprintf(MyStdOut, "Bad choice! :-(\n");
			par = 0;
		}
	}
	return par;
}


