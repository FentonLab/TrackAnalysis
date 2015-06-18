#include	<stdio.h>
// #include	"Track.h"

char	*GetLine(FILE *f, char *s)
{
	int	i = 0;

	*s = '\0';
	while (fscanf(f, "%c", s + i) != EOF){
		if(*(s + i) == '\n'){
			*(s + i + 1) = '\0';
			return s;
		}
		++i;
	}
	*(s + i) = '\0';
	return s;
}

int	GetStringInQuotes(char *Line, char *String, int *LineIndex){
	// returns the index of the Line indicating the last read character
	// String will contain the series of characters between a pair of double quotes ie. "This is a string"
	// if an open quote but no close quote is found then String contains all the characters since the open quote and EOF is returned.
	// if no open quote is found then String will contain only the first word and the appropriate index is returned

	int	n, i = 0, index;
	char 	c;

	*String = '\0';
	index = 0;
	while ((n = sscanf(Line+index, "%c", &c)) == 1){
		index++;
		if(c == '"'){ // found an open quote
			i = 0;
			while ((n = sscanf(Line+index, "%c", String+i)) == 1){
				index++;
				if(*(String+i) == '"'){ // found a close quote
					*(String+i) = '\0'; // terminate the text in the quotes as a string 
					*LineIndex = index;
					return  index;
				}
				i++;
			}
			return EOF; // did not find a close quote
		}
	}
	// could not find open quote
	// return the first white space delineated string 

	*String = '\0';
	index = 0;
	n = sscanf(Line, "%s%n", String, &index);
	if(n != EOF){
		*LineIndex = index;
		return index;
	}else
		return EOF;
}
