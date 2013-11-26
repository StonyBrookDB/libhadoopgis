#! /usr/bin/python

import sys


def main():

    if len(sys.argv) < 2:
        sys.stderr.write("Usage: "+ sys.argv[0] + "\n")
        sys.exit(1)

    i = 0
    dic = {} 
    for f in sys.argv:
      dic[f.strip()] = i
      i +=1

# this has to be correspond to the file name
    tag = dic[os.environ["map.input.file"]] 
    
    for line in sys.stdin:
        print "\t".join((tag,line.strip()))
    
    sys.stdout.flush()

if __name__ == '__main__':
    main()

