
#include <string.h>


// Filled in with the matched character
char misc_digtoken_optchar = 0;

char *misc_digtoken(char **string,char *match)
{

	misc_digtoken_optchar = 0;

	if(string && *string && **string) {

		// Skip "whitespace", anything in 'match' until we have something
		// that doesn't.
		// However, if we hit a " we stop asap, and match only to next ".


		while(**string && strchr(match,**string)) {

			// If a char is the quote, and we match against quote, stop loop
			(*string)++;

		}


		//printf(" found %c\n", **string);
		// If the first char is now a quote, and we match against quotes:
		if (!strchr(match, '"') && (**string == '"')) {

			//printf("  hack\n");
			// Skip the quote we are on
			(*string)++;

			// change match to only have a quote in it, that way we will
			// only match another quote.
			match = "\"";

		}


		if(**string) { /* got something */
			char *token=*string;

			if((*string=strpbrk(*string,match))) {

				misc_digtoken_optchar = *(*string);

				// If we match " to we force it to only look for
				// matching " and not any of the alternatives?

				*(*string)++=(char)0;
				while(**string && strchr(match,**string))
					(*string)++;

			}  else
				*string = ""; /* must be at the end */

			return(token);
		}
	}
	return((char *)0);
}

