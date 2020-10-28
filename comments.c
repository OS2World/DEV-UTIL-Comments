#ifndef lint
static char sccsid[] = "@(#)comments.c	1.4 11/23/90";
#endif
/*
**                                                                
**  Subsystem:   Source Configuration Management Tools
**  File Name:   comments.c               
**                                                        
** This software is Copyright (c) 1988, 1989, 1990 by Kent Landfield.
**
** Permission is hereby granted to copy, distribute or otherwise 
** use any part of this package as long as you do not try to make 
** money from it or pretend that you wrote it.  This copyright 
** notice must be maintained in any copy made.
**
** Use of this software constitutes acceptance for use in an AS IS 
** condition. There are NO warranties with regard to this software.  
** In no event shall the author be liable for any damages whatsoever 
** arising out of or in connection with the use or performance of this 
** software.  Any use of this software is at the user's own risk.
**
**  If you make modifications to this software that you feel 
**  increases it usefulness for the rest of the community, please 
**  email the changes, enhancements, bug fixes as well as any and 
**  all ideas to me. This software is going to be maintained and 
**  enhanced as deemed necessary by the community.
**		
**		Kent Landfield
**		kent@sparky.IMD.Sterling.COM
**
**  History:
**	Creation: Tue Feb 21 08:52:35 CST 1989 due to necessity.
**	
**
**	  NAME
**	       comments	- extract comment and non-blank	lines from C source
**	       files
**
**	  SYNOPSIS
**	       comments	[-c  [-v] [-tfilename] ] [-j] [-{Rrs}] file
**
**	  DESCRIPTION
**	       Comments is a general purpose tool for dealing with
**	       comment lines in C files.  comments also identifies
**	       Embedded comment and Delimiter mismatch lines while
**	       displaying comment lines of a file.
**
**	       The output from comments	goes to	standard out.
**
**	       If no options are used the comment lines	are output as they
**	       appear in the file or taken from	standard in.
**
**	       The meanings of the options are:
**
**	       -c   Produce code/comment/total statistics. The information
**		    is displayed in 2 rows.  The first row contains the
**		    number of characters of code, comment, and total.  The
**		    second row contains	the number of lines with code, com-
**		    ments, and total.  The second row also includes the
**		    name of the	file being processed.
**
**		    Special characters (ie. form and line feeds) are
**		    included only in total counts therefore the	number of
**		    characters of code plus comments do	not equal the total
**		    number of characters.  This	prevents blank lines from
**		    affecting the counts.
**
**		    A line with	both code and a	comment	counts as one code
**		    line and one comment line.	Therefore the number of
**		    lines with code plus lines with comment may	be greater
**		    than the total lines.
**
**	       -j   Left justify the comments printed.
**
**	       -R   Strip comments removing all	blank lines.
**
**	       -r   Strip comments removing multiple blank lines.
**
**	       -s   Strip comments leaving all blank lines.
**
**	       -tfilename
**		    The	filename to display with -c if taken from standard
**		    input.  This option	must be	used in	conjunction with
**		    the	-c option for it to be active.
**
**	       -v   Print out the header for count option.  Three columns
**		    from the -c	option are labeled:
**
**			  Code	   Comment    Total
**
**		    This option	must be	used in	conjunction with the -c
**		    option for it to be	active.
**
**	       file File to be processed if not	from standard input.
**
*/

#include <stdio.h>
#include <ctype.h>

#define EMBED_ERROR(a) (void) printf("Embeded comment,line %ld\n",a);
#define DELIM_ERROR(a) (void) printf("Delimiter mismatch, line %ld\n",a);

char *progname;             /* executable name */
char *flname;               /* input file name */

int strip = 0;              /* strip comments flag                   */
int remove_multiple = 0;    /* strip and remove multiple blank lines */
int remove_all = 0;         /* strip and remove all blank lines      */
int countit = 0;            /* count code/comment/total flag         */
int verbose = 0;            /* verbose output flag for header lines  */
int justify = 0;            /* left justify comments flag            */

main(argc, argv)
int argc;
char **argv;
{
   register int i;
   FILE *fp;

#ifdef  __EMX__
   _wildcard(&argc, &argv) ;    /* Expand arguments on EMX */
#endif

   flname = "STDIN";       /* assume input as standard input   */
   progname = argv[0];     /* save the program executable name */

   while (argc > 1 && argv[1][0] == '-') {
      switch (argv[1][1]) {
      case 'R':    /* strip comments removing all blank lines */
         remove_all = 1;
         strip = 1;
         countit = 0;
         break;
      case 'r':    /* strip comments removing multiple blank lines */
         remove_multiple = 1;     
      case 's':    /* strip comments leaving all blank lines */
         strip = 1;
         countit = 0;
         break;
      case 'c':    /* produce code/comment/total statistics */
         strip = 0;
         countit = 1;
         break;
      case 'v':    /* print out the header for count function */
         verbose = 1;
         break;
      case 'j':    /* left justify the comments printed */
         justify = 1;
         break;
      case 't':    /* the filename if taken from standard input */
         flname = argv[2];
         argc--, argv++;
         break;
      default:
         (void) fprintf(stderr,"%s: unknown arg %s\n",progname,argv[1]);
         return(1);
      }
      argc--, argv++;
   }
   if (argc == 1)             /* input coming from standard input */
      comments(stdin);
   else {
      for (i = 1; i < argc; i++) {
         flname = argv[i];
         if ((fp = fopen(flname, "r")) == NULL) {
            (void) fprintf(stderr, "%s: can't open %s\n",
                argv[0], flname);
            return(1);

         }
         else {
            comments(fp);
            (void) fclose(fp);
         }
      }
   }
   return(0);
}

comments(stream)
FILE *stream;
{
   register char *bp;            /* current character pointer             */
   register long lineno = 0L;    /* current line counter                  */
   char buf[BUFSIZ];             /* input buffer, contains record read    */
   int end_comment = 0;          /* boolean indicating comment just ended */
   int print_line = 0;           /* boolean indicating line is to print   */
   int incomment = 0;            /* boolean indicating if in a comment    */
   int inliteral = 0;            /* boolean indicating if in a literal    */
   int last_char = ' ';          /* holds the last item bp pointed at     */

   /*
   ** Determine if the strip or count function is requested to be 
   ** applied to the stream. If so, call the appropriate function
   ** and return to main.  
   */

   if (strip) {
      strip_comments(stream);
      return;
   }
   else if (countit) {
      count(stream);
      return;
   }

   /*
   ** Comments only will be printed.
   */

   while (fgets(buf, sizeof(buf), stream) != NULL) {
      bp = buf;
      lineno++;

      for (; *bp != '\n'; bp++) {
         if (*bp == '/' && !inliteral) {
             if (*(bp+1) == '*') {     /* begin comment */
                if (!incomment)        /* am I already in one */
                   incomment = 1;      /* no - indicate i am now */
                else                   /* yes - print an error */
                   EMBED_ERROR(lineno);
             }
             else if (last_char == '*') { /* end comment */
                if (incomment) {       /* I should be in a comment */
                   incomment = 0;      /* turn off comment flag    */
                   end_comment = 1;    /* indicate turnoff         */
                   print_line = 1;     /* indicate print needed.   */
                }
                else                   /* not in a comment - error */
                   DELIM_ERROR(lineno);
             }
         }
         else if (*bp == '"' && !incomment &&       /* turn on and  */
            (last_char != '\'' && *(bp+1) != '\'')) /*   off the    */
            inliteral = !inliteral;                 /* literal flag */

         if (!incomment && !end_comment) 
             *bp = ' ';                /* blank out non-comments    */
         last_char = *bp;              /* store the last char value */
         end_comment = 0;
       }

       /* newline found, loop terminated */

       if (incomment || print_line) {  /* printable comment line    */
           bp = buf;                   /* point at start of record  */
           if (justify) {              /* if justify, strip leading */
              while(*bp == ' ' || *bp == '\t') /* tabs and spaces   */
                  bp++;
           }
           (void) fputs(bp, stdout);   /* print the comment line    */
           print_line = 0;             /* turn off the print flag   */
       }
    }
   return;
}

strip_comments(stream)
FILE *stream;
{
   register char *bp;            /* current character pointer             */
   register long lineno = 0L;    /* current line counter                  */
   char buf[BUFSIZ];             /* input buffer, contains record read    */
   int incomment = 0;            /* boolean indicating if in a comment    */
   int inliteral = 0;            /* boolean indicating if in a literal    */
   int data_inline = 0;          /* boolean indicating if data in line    */
   int last_line_blank = 0;      /* boolean indicating if last line blank */
   int last_char = ' ';          /* holds the last item bp pointed at     */

   /*
   ** Strip comments from the stream and print according the
   ** the options set by the user.
   */

   while (fgets(buf, sizeof(buf), stream) != NULL) {
      bp = buf;
      lineno++;

      for (; *bp != '\n'; bp++) {
         if (*bp == '/' && !inliteral) {
             if (*(bp+1) == '*') {     /* begin comment              */
                if (!incomment)        /* am I already in one        */
                   incomment = 1;      /* no - indicate i am now     */
                else                   /* yes - print an error       */
                   EMBED_ERROR(lineno);
             }
             else if (last_char == '*') {  /* end comment            */
                if (incomment) {       /* I should be in a comment   */
                   incomment = 0;      /* turn off comment flag      */
                   *bp = ' ';          /* blank out the comment      */
                }
                else                   /* not in a comment - error   */
                   DELIM_ERROR(lineno);
             }
         }
         else if (*bp == '"' && !incomment &&        /* turn on and  */
            (last_char != '\'' && *(bp+1) != '\''))  /*   off the    */
            inliteral = !inliteral;                  /* literal flag */

         last_char = *bp;                /* store last char value    */
         if (incomment)
             *bp = ' ';                  /* blank out the comment    */
       }

       /* newline found, loop terminated */

       if (!incomment) {                /* time to check for data    */
          bp = buf;
          data_inline = 0;
          while (*bp && !data_inline) { /* determine if a blank line */
            if (*bp != '\t' && *bp != ' ' && *bp != '\n' && *bp != '\f')
                data_inline = 1;
             bp++;
          }
          if (data_inline) {             /* was real data found ?        */
             (void) fputs(buf, stdout);  /*   print out the line         */
             last_line_blank = 0;        /* indicate last line not blank */
          }
          else if (!remove_all) {        /* did user want blank lines ?  */
              if (!remove_multiple)      /* squash multiple blank lines? */
                 (void) fputs(buf, stdout);  /* no print the line        */
              else  if (!last_line_blank) {  /* yes - was last blank ?   */
                 (void) fputs(buf, stdout);  /*     print the line       */
                 last_line_blank = 1;        /* indicate last line blank */
              }
          }
       }
    }
}

count(stream) 
    FILE *stream;
{
   register char *bp;            /* current character pointer             */
   register long code_chars = 0L; /* number of characters of code         */
   register long code_lines = 0L; /* number of lines of code              */
   register long comment_chars = 0L; /* number of characters of comments  */
   register long comment_lines = 0L; /* # of lines containing comments    */
   register long chars = 0L;     /* total number of characters            */
   register long lineno = 0L;    /* total number of lines                 */
   char buf[BUFSIZ];             /* input buffer, contains record read    */
   int end_comment = 0;          /* boolean indicating comment just ended */
   int incomment = 0;            /* boolean indicating if in a comment    */
   int inliteral = 0;            /* boolean indicating if in a literal    */
   int had_comment = 0;          /* boolean indicating line had a comment */
   int had_data = 0;             /* boolean indicating line had data on it*/
   int last_char = ' ';          /* holds the last item bp pointed at     */

    while (fgets(buf, sizeof(buf), stream) != NULL) {
        lineno++;
        for (bp = buf; *bp != '\n'; bp++) {
             chars++;

             /* check for valid data on line */

             if (*bp != '/' && *bp != '*' && !incomment &&
                 *bp != '\t' && *bp != ' ' && *bp != '\f')
                had_data = 1;             /* set data on line flag */

             /* encountered a possible comment delimiter */

             if (*bp == '/' && !inliteral && 
                (*(bp+1) == '*' || last_char == '*')) { 
                incomment = !incomment;
                if (!incomment) {
                   had_comment = 1;    /* indicate comment on line */
                   end_comment = 1;    /* indicate comment stopped */
                   comment_chars++;    /* count last '/'           */
                }
             }

             /* encountered a possible literal delimiter */

             else if (*bp == '"' && !incomment && 
                (last_char != '\'' && *(bp+1) != '\''))
                inliteral = !inliteral;

             last_char = *bp;

             if (incomment)
                 comment_chars++;         
             else if (!end_comment)
                 code_chars++;
             end_comment = 0;
        }
  
        /* newline found - loop terminated */
        chars++;          /* add the newline character to the total */
#ifdef COUNT_NEWLINES
        if (incomment)
            comment_chars++;         
        else
            code_chars++;
#endif

        if (incomment || had_comment)
            comment_lines++;

        if (had_data)
            code_lines++;

        had_comment = 0;
        had_data = 0;
    }
    if (verbose)
       (void) printf(" Code     Comment   Total\n");

    (void) printf("%5ld%10ld%10ld\n%5ld%10ld%10ld      %s\n",
                 code_chars, comment_chars, chars,
                 code_lines, comment_lines, lineno, flname);
}
/* this is the last line of the file */


