# In order to process the files in this directory, you'll need
# yodl, an excellent text-processing tool:
# http://www.xs4all.nl/~jantien/yodl/
# Pre-generated files are, of course, included.

all: man html
man:
	yodl2man ahwmrc.yo
html:
	yodl2html ahwmrc-html.yo
clean:
	rm -f *~ *.html *.man
