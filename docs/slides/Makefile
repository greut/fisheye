file = fisheye

${file}.pdf: ${file}.tex
	xelatex ${file}.tex
	xelatex ${file}.tex

### in case you have a bibliography
bib:
	bibtex ${file}

### 2 slides per page
2: ${file}.pdf
	pdftops ${file}.pdf -expand - | psnup -2 -d -b 1mm -m 2cm -l | ps2pdf - ${file}_2.pdf

### 4 slides per page
4: ${file}.pdf
	pdftops ${file}.pdf -expand - | psnup -4 -d -b 1mm -m 2cm -l | ps2pdf - ${file}_4.pdf

### 8 slides per page
8: ${file}.pdf
	pdftops ${file}.pdf -expand - | psnup -8 -d -b 1mm -m 2cm -l | ps2pdf - ${file}_8.pdf


clean:
	rm -f ${file}.{out,log,aux}
	rm -f includes/*.{out,log,aux}

realclean: clean
	rm -f ${file}.pdf
	rm -f ${file}_*.pdf
	

distr:
	cd ..; tar cvfz slides.tgz slides/
