SRC_FILE = DesignDoc.tex
BUILD_DIR = build
PDF_NAME = DesignDoc


all: $(SRC_FILE) | OutputDir
	pdflatex -jobname=$(PDF_NAME) $(SRC_FILE)
	mv *.aux *.log  $(BUILD_DIR)

OutputDir:
	mkdir -p $(BUILD_DIR)

.PHONY: clean
clean:
	rm -rf ./$(BUILD_DIR)
	rm -rf $(PDF_NAME).pdf

.PHONY: view 
view:
	evince $(PDF_NAME).pdf &
