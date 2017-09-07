import random
import sys
import os

def random_char():
    return chr(ord('a')+random.randint(0,25))

def random_word(n=None):
    n = n or random.randint(1,10)
    return ''.join(random_char() for k in range(n))

def random_text(m=None):
    m = m or random.randint(1000,10000)
    return ' '.join(random_word() for k in range(m))

def generate_data(n=10,m=100000):
    if not os.path.exists('files'): os.mkdir('files')
    filenames = []
    for k in range(n):
        filename = 'files/%s.txt' % k
        text = random_text(m)
        open(filename,'w').write(text)
        filenames.append(filename)
    return filenames

def main():
    filenames = sys.argv[1:]
    s = raw_input('search:')
    words = s.split()
    for filename in filenames:
        text = open(filename).read()
        if all(word in text for word in words):
            print filename

generate_data()
main()   
