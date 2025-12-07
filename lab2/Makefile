## Simple Makefile

# Convert lab2.md to lab2.pdf
lab2.pdf: lab2.md
	pandoc $< -o $@ --pdf-engine=xelatex

# Clean up generated PDFs
clean:
	rm -f *.pdf