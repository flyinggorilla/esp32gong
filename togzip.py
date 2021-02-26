import gzip
import sys

try: 
    if len(sys.argv) < 3:
        print("usage: gzip.py <inputfile> <outputfile>")
        quit()
    inputFilename = sys.argv[1]
    gzipFilename = sys.argv[2]
    src = open(inputFilename, "rb") 
    print("Gzip-Compressing file '{}' to '{}'.".format(inputFilename, gzipFilename)) 
    gzf = gzip.GzipFile(filename=gzipFilename, mode='wb')
    gzf.write(src.read())
    src.close()
    gzf.close()
    
except IOError:
    print("cannot open ", inputFilename, "for reading or ", gzipFilename, " for writing")



