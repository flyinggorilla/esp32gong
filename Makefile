#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := esp32gong

all: main/indexhtml.h main/fontttf.h main/fontsvg.h main/fonteot.h main/fontwoff.h main/wavdata.h 

main/indexhtml.h: 
	python data2h.py data/index.html main/indexhtml.h

main/fontttf.h: 
	python data2h.py data/material-design-icons.ttf main/fontttf.h
	
main/fontwoff.h: 
	python data2h.py data/material-design-icons.woff main/fontwoff.h
	
main/fontsvg.h: 
	python data2h.py data/material-design-icons.svg main/fontsvg.h

main/fonteot.h: 
	python data2h.py data/material-design-icons.eot main/fonteot.h

main/wavdata.h: 
	python data2h.py data/gong.wav main/wavdata.h

include $(IDF_PATH)/make/project.mk



