## Simple Makefile

# Convert lab2.md to lab2.pdf
lab3.pdf: lab3.md
	pandoc $< -o $@ --pdf-engine=xelatex

# Clean up generated PDFs
clean:
	rm -f *.pdf