#!/usr/bin/env python
import sys, os, subprocess, StringIO, collections, random, getpass, getent, shelve
import shelve, os, fcntl, new
import __builtin__
from fcntl import LOCK_SH, LOCK_EX, LOCK_UN, LOCK_NB

def _close(self):
    shelve.Shelf.close(self)
    fcntl.flock(self.lckfile.fileno(), LOCK_UN)
    self.lckfile.close()

def shopen(filename, flag='c', protocol=None, writeback=False,
           block=True, lckfilename=None):
    """Open the sheve file, createing a lockfile at filename.lck.  If 
    block is False then a IOError will be raised if the lock cannot
    be acquired"""
    if lckfilename == None: lckfilename = filename + ".lck"
    lckfile = __builtin__.open(lckfilename, 'w')
    lockflags = LOCK_NB if not block else LOCK_SH if flag == 'r' else LOCK_EX
    fcntl.flock(lckfile.fileno(), lockflags)
    shelf = shelve.open(filename, flag, protocol, writeback)
    shelf.close = new.instancemethod(_close, shelf, shelve.Shelf)
    shelf.lckfile = lckfile 
    return shelf

def main():
    if len(sys.argv)<2:
        print 'Usage: grade <filename.py>'
        getpass.getpass('press enter to continue...')
        return
    os.system('hg addremove .')
    os.system('hg commit -m "grading %s"' % sys.argv[1])
    path = os.path.join(os.getcwd(),sys.argv[1])
    if len(sys.argv)==4:
        classname, student_id = sys.argv[2], sys.argv[3]
    else:
        classname, student_id = path.split('/')[2].split('-')
    username = path.split('/')[2]
    name = getent.passwd(username).gecos.rstrip(',')
    print name
    grade = Test.run(classname, path)    
    if grade!=None:
        key = username+': '+name
        grades = shopen('/tmp/grades')
        u = grades[classname] = grades.get(classname,{})
        s = u[key] = u.get(key,{})
        s[path.split('/')[-1]] = grade
        grades[classname] = u
        #print grades
        grades.close()
    getpass.getpass('press enter to continue...')

def testall(classname, student_id, programname):
    args = 'python3 a1p1.py'
    text = 'hello world'
    p = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                         stderr = subprocess.PIPE, shell = True)
    output, error = p.communicate(text)
    print output
    print error

class Test(object):
    tests = collections.defaultdict(list)
    def __init__(self, classname, programname, repeats=1, grade=2, python='python'):
        self.classname = classname
        self.programename = programname
        self.repeats = repeats
        self.grade = grade
        self.python = python
    def __call__(self,f):
        Test.tests[(self.classname,self.programename)].append(
            dict(f=f,repeats=self.repeats,grade=self.grade,python=self.python))                 
    @staticmethod
    def run(classname, path):
        programname = path.split('/')[-1]
        key = (classname,programname)
        if not key in Test.tests:
            print 'Nothing to do with this file'
            return None
        else:
            grade = 0
            for test in Test.tests[key]:
                errors = 0
                trials = test['repeats']
                successes = 0
                error = None
                for k in range(trials):
                    text_in,text_out = test['f']()
                    p = subprocess.Popen('%s %s' % (test['python'], path), 
                                         stdin=subprocess.PIPE,
                                         stdout=subprocess.PIPE,
                                         stderr = subprocess.PIPE, shell = True)
                    output, error = p.communicate(text_in)                    
                    success = not error and all(line in output
                                                for line in text_out.split('\n'))
                    if success:
                        successes+=1
                    elif not errors:
                        errors = True
                        print 'TRIED INPUT'
                        print text_in
                        if error:
                            print 'ERROR'
                            print error
                        else:
                            print 'EXPECTED CORRECT OUTPUT'
                            print text_out
                            print 'RECEIVED INCORRECT OUTPUT'
                            print output                        
                grade += max(0,int(test['grade']*successes/trials))
            print 'Your grade is', grade
            return grade

@Test('csc241','a1p1.py',repeats=10,python='python3',grade=2)
def test1():
    """
    Write a program that asks you to input a number n (an integer)
    and prints the squares of all numbers between 0 and n.
    """
    n = random.randint(10,20)
    return str(n), '\n'.join(str(i**2) for i in range(0,n))

@Test('csc241','a1p2.py',repeats=10,python='python3',grade=2)
def test1():
    """
    Write a program that asks you to input a string and prints the string reversed
    """
    s = ''.join(chr(random.randint(0,25)+ord('a')) for k in range(8))
    rs = ''.join(s[k] for k in range(len(s)-1,-1,-1))
    return s, rs

@Test('csc241','a1p3.py',repeats=10,python='python3',grade=2)
def test1():
    """
    Write a program that lets you input a string and prints the string after
    replacing all letters 'a' with letter 'o' and all letters 'o' with letter 'a'
    """
    s =''.join(chr(random.randint(0,25)+ord('a')) for k in range(16))
    rs = ''.join({'a':'o','o':'a'}.get(c,c) for c in s)
    return s, rs

@Test('csc241','a1p4.py',repeats=10,python='python3',grade=2)
def test1():
    """
    Write a program that inputs a string and returns the string converted 
    to morse code (leaving spaces invariant).
    For example input 'aa bb' should produce output '.-.- -...-...'
    """
    CODE = {'A': '.-',     'B': '-...',   'C': '-.-.', 
            'D': '-..',    'E': '.',      'F': '..-.',
            'G': '--.',    'H': '....',   'I': '..',
            'J': '.---',   'K': '-.-',    'L': '.-..',
            'M': '--',     'N': '-.',     'O': '---',
            'P': '.--.',   'Q': '--.-',   'R': '.-.',
            'S': '...',    'T': '-',      'U': '..-',
            'V': '...-',   'W': '.--',    'X': '-..-',
            'Y': '-.--',   'Z': '--..',    ' ':' '}
    text_in = ' '.join(''.join(random.choice(CODE.keys()) for i in range(6))
                       for j in range(2)).lower()
    text_out = ''.join(CODE[c] for c in text_in.upper())
    return text_in, text_out

if __name__=='__main__': main()
