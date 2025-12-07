pdfName = FinalReport 
buildDir = build
srcFile = main.tex

all: create

create: $(srcFile) | createBuildDir
	pdflatex $(srcFile) 
	mv *.aux *.log *.out $(buildDir)

createBuildDir:
	mkdir -p $(buildDir)

.PHONY: clean view
view: 
	evince *pdf &

clean:
	rm -rf ./*.pdf
	rm -rf ./$(buildDir)
