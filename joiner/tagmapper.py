import sys
import os

def main():

    if len(sys.argv) < 2:
        sys.stderr.write("Usage: "+ sys.argv[0] + "\n")
        sys.exit(1)

    i = 0
    dic = {}
    for f in sys.argv:
      dic[f.strip()] = i
      i +=1

    fullfilename = os.environ['map_input_file']

    if fullfilename.find('/') != -1:
        filename = fullfilename[fullfilename.rfind('/')+1:]
    else:
        filename = fullfilename
    tag = dic[filename]

    for line in sys.stdin:
        arr = line.strip().split("\t")
        print "\t".join((arr[0], str(tag), "\t".join(arr[1:])))

    sys.stdout.flush()

if __name__ == '__main__':
    main()
