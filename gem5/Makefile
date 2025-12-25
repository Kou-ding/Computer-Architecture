## Simple Makefile

# Convert markdown to PDF
gem5.pdf: gem5.md
	pandoc $< -o $@ --pdf-engine=xelatex

# Clean up generated PDFs
clean:
	rm -f *.pdf