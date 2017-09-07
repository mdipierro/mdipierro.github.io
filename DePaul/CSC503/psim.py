"""

# Created by Massimo Di Pierro # 2007

class PSIM:
    def __init__(self,n,logfile=None,topology=SWITCH): ...
    # allowed topologies: BUS, SWITCH, MESH1(p), TORUS1(p),
    #                     MESH2(p), TORUS2(p), TREE
    def rank(self): ...
    def nprocs(self): ...
    def topology(self,i,j): ...
    def send(self,j,data): ...
    def receive(self,j): ...
    def one2all_broadcast(self, source, value): ...
    def all2one_collect(self,destination,value): ...
    def all2all_broadcast(self, value): ...
    def all2one_reduce(self,destination,value,op=SUM): ...
    # allowed global ops: SUM, MUL, MAX, MIN
    def all2all_reduce(self,value,op=SUM): ...        
    def barrier(self): ...
"""
import os, string, cPickle, time, math

def BUS(i,j):
    return True

def SWITCH(i,j):
    return True

def MESH1(p):
    return lambda i,j,p=p: (i-j)**2==1

def TORUS1(p):
    return lambda i,j,p=p: (i-j+p)%p==1 or (j-i+p)%p==1

def MESH2(p):
    q=int(math.sqrt(p)+0.1)
    return lambda i,j,q=q: ((i%q-j%q)**2,(i/q-j/q)**2) in [(1,0),(0,1)]

def TORUS2(p):
    q=int(math.sqrt(p)+0.1)
    return lambda i,j,q=q: ((i%q-j%q+q)%q,(i/q-j/q+q)%q) in [(0,1),(1,0)] or \
                           ((j%q-i%q+q)%q,(j/q-i/q+q)%q) in [(0,1),(1,0)]

def TREE(i,j):
    return i==int((j-1)/2) or j==int((i-1)/2)

def SUM(x,y): return x+y

def MUL(x,y): return x*y

def MAX(x,y): return max(x,y)

def MIN(x,y): return min(x,y)

class PSIM:

    def log(self,message):
        """
        logs the message into self._logfile
        """
        if self._logfile!=None:
            self._logfile.write(message)
            pass
        return

    def __init__(self,n,logfile=None,topology=SWITCH):
        """
        forks n-1 processes and creates n*n 
        """
        self._logfile=logfile
        self._topology=topology
        self.log("START: creating %i parallel processes\n" % (n))
        self._nprocs=n        
        self.pipes={}
        for i in range(n):
            for j in range(n):
                k=i*n+j
                self.pipes[k]=os.pipe()
                pass
            pass
        i=0
        while i<n-1:
            if os.fork():
                break
            else:
                i=i+1
                pass
            pass
        self._rank=i
        self.log("START: done.\n")

    def __del__(self):
        """
        end of parallel processing
        """
        self.log("END: process %i terminating.\n" % (self._rank))
        return

    def rank(self):
        """
        returns the rank of the process
        """
        return self._rank

    def nprocs(self):
        """
        returns the number of parallel processes
        """
        return self._nprocs

    def topology(self,i,j):
        return self._topology(i,j)

    def _send(self,j,data):
        """
        sends data to process #j
        """
        if j<0 or j>=self.nprocs():
            self.log("process %i: send(%i,...) failed!\n" % (self.rank(),j))
            raise Exception
        self.log("process %i: send(%i,%s) starting...\n" % (self.rank(),j,repr(data)))
        k=self._rank*self.nprocs()+j
        s=cPickle.dumps(data)
        os.write(self.pipes[k][1],string.zfill(str(len(s)),10))
        os.write(self.pipes[k][1],s)
        self.log("process %i: send(%i,%s) success.\n" % (self.rank(),j,repr(data)))

    def send(self,j,data):
        if not self.topology(self.rank(),j):
            raise Exception
        self._send(j,data)

    def _receive(self,j):
        """
        returns the data received from process #j 
        """
        if j<0 or j>=self.nprocs():
            self.log("process %i: receive(%i) failed!\n" % (self._rank,j))
            raise Exception
        self.log("process %i: receive(%i) starting...\n" % (self._rank,j))
        k=j*self.nprocs()+self.rank()
        try:
            size=int(os.read(self.pipes[k][0],10))
            s=os.read(self.pipes[k][0],size)
        except:
            self.log("process %i: COMMUNICATION ERROR!!!\n" % (self._rank))
            sys.exit(1)
            pass
        data=cPickle.loads(s)
        self.log("process %i: receive(%i) done.\n" % (self._rank,j))
        return data

    def receive(self,j):
        if not self.topology(self.rank(),j):
            raise Exception
        return self._send(j)
        

    def one2all_broadcast(self, source, value):
        self.log("process %i: BEGIN one2all_broadcast(%i,%s)\n" % (self._rank,source, repr(value)))
        if self._rank==source:
            for i in range(0, self.nprocs()):
                if i!=source:
                    self._send(i,value)
                    pass
                pass
            pass
        else:
            value=self._receive(source)
            pass
        self.log("process %i: END one2all_broadcast(%i,%s)\n" % (self._rank,source, repr(value)))
        return value

    def all2one_collect(self,destination,value):
        self.log("process %i: BEGIN all2one_collect(%i,%s)\n" % (self._rank,destination,repr(value)))
        vector=[]
        self._send(destination,value)
        if self.rank()==destination:
            for i in range(self.nprocs()):
                vector.append(self._receive(i))
        self.log("process %i: END all2one_collect(%i,%s)\n" % (self._rank,destination,repr(value)))
        return vector

    def all2all_broadcast(self, value):
        self.log("process %i: BEGIN all2all_broadcast(%s)\n" % (self._rank, repr(value)))
        vector=self.all2one_collect(0,value)
        vector=self.one2all_broadcast(0,vector)        
        self.log("process %i: END all2all_broadcast(%s)\n" % (self._rank, repr(value)))
        return vector
            
    def all2one_reduce(self,destination,value,op=SUM):        
        self.log("process %i: BEGIN all2one_reduce(%s)\n" % (self._rank,repr(value)))
        result=None
        self._send(destination,value)
        if self.rank()==destination:
            result=self._receive(0)        
            for i in range(1,self.nprocs()):
                result=op(result,self._receive(i))        
        self.log("process %i: END all2one_reduce(%s)\n" % (self._rank,repr(value)))    
        return result

    def all2all_reduce(self,value,op=SUM):        
        self.log("process %i: BEGIN all2all_reduce(%s)\n" % (self._rank,repr(value)))
        result=self.all2one_reduce(0,value,op)
        result=self.one2all_broadcast(0,result)
        self.log("process %i: END all2all_reduce(%s)\n" % (self._rank,repr(value)))    
        return result

    def barrier(self):        
        self.log("process %i: BEGIN barrier()\n" % (self._rank))
        self.all2all_broadcast(0)
        self.log("process %i: END barrier()\n" % (self._rank))    
        return

__author__ = 'Massimo Di Pierro'
__version__= 'version 2.0'

def test():
    comm=PSIM(5,None,SWITCH)
    if comm.rank()==0: print 'start test'
    a=sum(comm.all2all_broadcast(comm.rank()))
    comm.barrier()
    b=comm.all2all_reduce(comm.rank())
    if a!=10 or a!=b:
        print 'from process', comm.rank()
        raise Exception
    if comm.rank()==0: print 'test passed'

def test_topology():
    comm=PSIM(9,None,TORUS2(9))
    i=comm.rank()
    for j in range(comm.nprocs()):        
        time.sleep(1.0*i+0.1*i)     
        if i!=j and comm.topology(i,j):
            print i,'-',j

    
if __name__=='__main__': test()

