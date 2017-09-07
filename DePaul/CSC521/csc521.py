from math import *
from time import *
from random import *
from Tkinter import *
from copy import *
from csc521vrml import *

true=1
false=0

################################################################
# Chapter 1 Examples
################################################################


def pi_taylor(n):
    """
    Takes n (int) and computes pi using n terms of the
    Taylor expansion of pi=4 arctan(1)
    """
    pi=0
    for i in range(n):
        pi=pi+4.0/(2*i+1)*(-1)**i
        print i,pi

def pi_mc(n):
    """
    Takes n (int) and computes pi using n Monte Carlo points
    """
    pi=0
    counter=0
    for i in range(n):
        x=random()
        y=random()
        if x**2 + y**2 < 1:
            counter=counter+1
        pi=4.0*float(counter)/(i+1)
        print i,pi

def daily_income(N,days=1000):
    """
    Takes N (int) and days (int) and simulated 'days' days of
    an online merchant. Each day the merchant keeps N items
    in stock an is visited by int(gauss(976,352)) visitors.
    Each visitor (j) has 5% probability of buying an item
    instock and 2% of buying an item not in stock.
    An item costs to the visitor $100. The same item costs
    $30 to the merchant to keep it in stock for one day.
    """
    income=0
    for i in range(days):
        instock=N        
        for j in range(int(gauss(976,352))):
            if instock>0:
                if random()<0.05:
                    instock=instock-1
                    income=income+70
            else: 
                if random()<0.02:
                    income=income+100
        income=income-30*instock
    return income/days


################################################################
# Mapping any memoryless distribution into uniform
################################################################

def exponential2bernoulli(data,i):
    """
    Maps data, a random sequence of continuum random variables drawn from
    the same memoryless distribution into a sequence of 0,1
    """
    if data[2*i]<data[2*i+1]: return 1
    return 0

def bernoulli2uniform(data,i,bits=5):
    """
    Maps a sequence of random '0' and '1' data="0101010011100..." into a
    a sequence of uniform random variables with 'bits' bits of precision.
    """
    out=0
    for j in range(0,bits):
        out=out+data[i*bits+j]*(0.5**(j+1))        
    return outm


################################################################
# Random number generators
################################################################

def random_mcg(x,a,m):
    """
    Simple multiplicative congruential generator.
    """
    return a*x % m

def test_random_mcg(a=65539,m=2**31):
    """
    The the randomMCG by producing 10 uniform random numbers.
    """
    x=int(time())
    print 'seed=',x
    for i in range(10):
        x=random_mcg(x,a,m)
        print float(x)/m

# test_random_mcg()        

def totient(m):
    """
    Computes the totient of m, i.e. the maximum possible period for a
    multiplicative congruential generator a*x % m.
    """
    t=1
    for p in [2]+range(3,int(m)+1,2):
        n=0
        while m%p==0:
            m=m/p
            n=n+1
        if n>0:
            print p,n
            t=t*(p**(n-1))*(p-1)
    if m!=1:
        p,n=m,1
        t=t*(p**(n-1))*(p-1)
    return t


################################################################
# Arbitrary precision arithmetics
################################################################

def libreak(a,bits):
    """
    Breaks a number a in words of 'bits' bits, for arbitrary precision aritmetics.
    """
    size=2**bits
    chunks=[]
    while a!=0:
        chunks.append(a % size)
        a=a/size
    return chunks

def mulmod(a,b,m,bits=8):
    """
    Computes a*b % m where a*b is larger than machine precision.
    """
    ac=libreak(a,bits)
    bc=libreak(b,bits)
    d=[1]
    for k in range(1,len(ac)+len(bc)):
        d.append(d[k-1]*(2**bits) % m)
    c=0
    for i in range(len(ac)):
        for j in range(len(bc)):
            k=i+j
            c=c+(ac[i]*bc[j] % m)*d[k]
            c=c % m
    return c

def summod(a,b,m,bits=8):
    """
    Computes a*b % m where a*b is larger than machine precision.
    """
    ac=libreak(a,bits)
    bc=libreak(b,bits)
    size=2**bits
    d=[1]
    for k in range(1,max(len(ac),len(bc))+1):
        d.append(d[k-1]*size % m)
    cc=[]
    reminder=0
    for k in range(max(len(ac),len(bc))):
        cc.append((reminder+ac[k]+bc[k]) % m)
        if cc[k]>size:
            reminder=int(cc[k]/size)
            cc[k]=cc[k]%size
    if reminder!=0:
        cc.append(reminder)
    c=0
    for k in range(len(cc)):
        c=c+mulmod(cc[k],d[k],m,bits)
    return c%m

# print summod(1000,1111,1000,3)

def powmod(a,n,m,bits=8):
    """
    Computes a**n % m where a*b is larger than machine precision.
    """
    if n==0:
        return 1
    elif n==1:
        return a
    if n%2==0:
        return powmod(mulmod(a,a,m,bits),n/2,m,bits)
    else:
        return mulmod(powmod(a,n-1,m,bits),a,m,bits)

################################################################
# Generic Multiplicative Congruential Generator
# default coefficents are Marsenne Generator
################################################################

class MCG:
    """
    Class tha implements a general multiplicative congruential generator
    """
    def __init__(self,seed=None,a=7**5,m=2**31-1): #constructor
        """
        Class constructor takes the see (int), a and m
        """
        if seed==None: seed=time()
        self.a=7**2 # Python: self.a; Java: this.a; C++: this->a
        self.m=2**31-1
        self.x=int(seed)

    def next(self):
        self.x=mulmod(self.a,self.x,self.m)

    def random(self):
        """
        Returns the next uniform pseudo-random number in (0,1)
        """
        self.next()
        return float(self.x)/self.m

    def randint(self,a,b):
        """
        Returns the next integer pseudo-random number in [a,b]
        """        
        return a+int((b-a+1)*self.random())

    def exponential(self,lamb):
        return -log(1-random())/lamb        

    def gaussian(self):
        try:
            if self.other==None: raise Exception
            this=self.other
            self.other=None
            return this
        except:
            while 1:
                v1=2*self.random()-1
                v2=2*self.random()-1
                s=v1**2+v2**2
                if s<=1: break
            this=sqrt(-2*log(s)/s)*v1
            self.other=sqrt(-2*log(s)/s)*v2
            return this

    def leapfrog(self,k):
        a=self.a
        m=self.m
        generators=range(k)
        for i in range(k):
            self.next()
            generators[i]=MCG(self.x,powmod(a,k,m),m)
        return generators
                              
def test_MCG():
    """
    Test the default Marsenne generator.
    """
    marsenne=MCG(time())
    for i in range(10):
        print marsenne.random()
    for i in range(10):
        print marsenne.randint(0,10)

################################################################
# Parallelization of random generators
################################################################

def test_leapfrog():
    """
    Creates 3 independet generator by leapfrogging and prints 5 numbers from
    each generator.
    """
    generators=MCG().leapfrog(3)
    for k in range(3):
        for i in range(5):
            x=generators[k].random()
            print k,'\t',i,'\t',x

# test_leapfrog()

################################################################
# Binning
################################################################

def bin(generator,nevents,a,b,nbins):
    """
    Uses generator to generate nevents uniform random numbers and bins their
    distribution in nbins ranging from a to b. It returns the bins.
    """
    # create empty bins
    bins=[]
    for k in range(nbins):
        bins.append(0)
    # fill the bins
    for i in range(nevents):
        x=generator.random()
        if x>=a and x<=b:
            k=int((x-a)/(b-a)*nbins)
            bins[k]=bins[k]+1
    # normalize bins
    for i in range(nbins):
        bins[i]=float(bins[i])/nevents
    return bins

def test_bin(nevents=1000,nbins=10):
    """
    Tests binning on a unifrom random number generator.
    """
    bins=bin(MCG(time()),nevents,0,1,nbins)
    for i in range(len(bins)):
        print i, bins[i]

# test_bin()        

################################################################
# Chi-squared
################################################################

def chi_square(nevents, frequencies, pis):
    """
    Given a set normalized bins pis[i] and a set of expected
    frequencies for those bins frequencies[i] is computes the chi-squared.
    A chi-square close to 0 indicates that measures probabilities follow
    expected frequencies.
    """
    sum=0
    for i in range(len(frequencies)):
        sum=sum+((frequencies[i]-pis[i])**2)/pis[i]
    return nevents*sum

#print chi_square(1000,[0.101, 0.117, 0.092, 0.091, 0.091, 0.122, 0.096, 0.102, 0.09, 0.098],[0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1])
	
#print chi_square(1000,[0.09926,0.09772,0.10061,0.09894,0.10097,0.09997,0.10056,0.09976,0.10201,0.1002],[0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1])

################################################################
# Draw from discrete distributions
################################################################

def discrete_map(u, table):
    """
    Maps a uniform random number u into one of the table items.
    Each table item has a value and its associated probability.
    """
    # O(n)
    for item,probability in table:        
        if u<probability:
            return item
        else:
            u=u-probability
    return item #program should not arrive here if sum table[1]

def test_discrete_map(nevents=100,table=[[0,0.50],[2,0.27],[1,0.23]]):
    generator=MCG(time())
    f=[0,0,0]
    for k in range(nevents):
        u=generator.random()
        p=discrete_map(u,table)
        print p,
        f[p]=f[p]+1
    print
    for i in range(len(table)):
        f[i]=float(f[i])/nevents
        print 'frequency[%i]=%f' % (i,f[i])

# test_discrete_map()

def log2(x): return log(x)/log(2)

class FishmanYarberry1993:
    def __init__(self,table=[[0,0.2], [1,0.5], [2,0.3]]):
        t=log2(len(table))
        while t!=int(t):        
            table.append([0,0.0])
            t=log2(len(table))
        t=int(t)
        a=[]
        for i in range(t):
            a.append([])
            if i==0:
                for j in range(2**t):
                    a[i].append(table[j][1])
            else:
                for j in range(2**(t-i)):
                    a[i].append(a[i-1][2*j]+a[i-1][2*j+1])
        self.table=table
        self.t=t
        self.a=a

    def discrete_map(self, u):
        """
        Maps a uniform random number u into one of the table items.
        Each table item has a value and its associated probability.
        Works as discrete_map(u,table) but runs in O(log n) and
        internal table is built once for all by constructor.
        """
        i=int(self.t)-1
        j=0
        b=0
        while i>0:
            if u>b+self.a[i][j]:
                b=b+self.a[i][j]
                j=2*j+2
            else:
                j=2*j
            i=i-1
        if u>b+self.a[i][j]:
            j=j+1
        return self.table[j][0]

def test_FishmanYarberry1993():
    """
    Compares two equivalent ways to draw from discrete distribution.
    """
    for k in range(2):
        u=random()
        print random_from_table(u,table=[['A',0.2], ['B',0.5], ['C',0.3]]),
        print FishmanYarberry1993(table=[['A',0.2], ['B',0.5], ['C',0.3]]).map(u)       

# test_FishmanYarberry1993()

def discrete_ar(generator, table):
    """
    Draws from a discrete probability distribution using accept reject
    """
    while 1:
        i=generator.randint(0,len(table)-1)
        u=generator.random()        
        if u<table[i]: return i

def test_discrete_ar(nevents=100,table=[0.50,0.23,0.27]):
    """
    Test accept reject
    """
    generator=MCG(time())
    f=[0,0,0]
    for k in range(nevents):
        p=discrete_ar(generator, table)
        print p,
        f[p]=f[p]+1
    print
    for i in range(len(table)):
        f[i]=float(f[i])/nevents
        print 'frequency[%i]=%f' % (i,f[i])

# test_discrete_ar()

def binomial_map(n,p,u=random()):
    """
    Maps a uniform random number u into an integer with binomial distribution
    p(k)=n!/(k!(n-k)!) p**k (1-p)**(n-k)
    """
    k=0
    probability=(1.0-p)**n
    while 1:
        if u<probability:
            return k
        else:
            u=u-probability
        k=k+1
        if k<=n:
            probability=probability*(n-k)/(i+k)*p/(1.0-p)
    return k-1

def Poisson_map(lamb,u=random(),epsilon=0):
    """
    Maps a uniform random number u into an integer with Poisson distribution
    p(k)= exp(-lamb) lamb**(k)/k!
    """
    k=0
    probability=exp(-lamb)
    while 1:
        if u<probability+epsilon:
            return k
        else:
            u=u-probability
        k=k+1
        probability=probability*lamb/(k+1)

################################################################
# Draw from continuum distributions
################################################################

def random_with_my_probability():
    return (8*random())**(1./0/3.0)

def continuum_ar(generator, p, a,b,c):
    while 1:
        x=a+(b-a)*generator.random()
        y=generator.random()
        if y<p(x)/c: return x

def random_point_in_domain(box, S, generator):
    """
    Returns a random point in the box delimited by S.
    """
    d=len(box)
    while 1:
        p=[]
        for i in range(d):
            p.append(generator.random()*(box[i][1]-box[i][0])+box[i][0])
        if S(p)==1:
            return p

################################################################
# Random points in multidimensional domain
################################################################
       
class MyCanvas:
    """
    Class to draw points on a canvas
    """
    def __init__(self,width=100,height=100):
        self.width=width
        self.height=height
        self.root=Tk()
        self.cs=Canvas(self.root,width=self.width,height=self.height)
        self.cs.pack(expand=1)
    def mark_point(self,x,y, color='black'):
        self.cs.create_oval(x,self.height-y,x,self.height-y,fill=color)
    def draw_line(self,x0,y0, x1,y1,color='black'):
        self.cs.create_line(x,self.height-y0,x1,self.height-y1,fill=color)
    
def example_circle(p):
    """
    Returns 1 if p is inside a circle of radius 200, 0 otherwise.
    """
    if p[0]**2+p[1]**2<200*2:
        return 1
    else:
        return 0

def example01():
    """
    Creates a canvas and creates 1000 random points inside the circle.
    """
    g=MCG(time())
    canvas=MyCanvas(200,200)
    for i in range(1000):
        p=random_point_in_domain([[0,200],[0,200]], example_circle,g)
        canvas.mark_point(p[0], p[1], 'red')
    
# example01()

################################################################
# ad-hoc methods to draw from continuum distribution
################################################################


################################################################
# Monte Carlo Integration
################################################################

def mc_integrate_1d(f,a,b,n,generator):
    sum1=0.0
    sum2=0.0   
    for i in range(n):
        x=a+(b-a)*generator.random()
        y=f(x)
        sum1=sum1+y
        sum2=sum2+y*y
        I=(b-a)*sum1/(i+1)
        variance=((b-a)**2)*sum2/(i+1)-I*I
        dI=sqrt(variance/(i+1))
        print I, dI
    return I,dI

def test_f(x):
    return sin(x)*(3.0-exp(-5-x))

def mc_integrate_1d_2(n,generator):
    sum1=0.0
    sum2=0.0   
    for i in range(n):
        x=sqrt(5.0*generator.random()+4)
        y=2.5*(2.0-exp(-x))
        sum1=sum1+y
        sum2=sum2+y*y
        I=sum1/(i+1)
        variance=sum2/(i+1)-I*I
        dI=sqrt(variance/(i+1))
        print I, dI
    return I,dI

# mc_integrate_1d_2(10000,MCG(time()))

def mc_integrate_nd(f,box,domain,volume,n,generator):
    sum1=0.0
    sum2=0.0
    for i in range(n):        
        x=random_point_in_domain(box,domain,generator)
        y=f(x)
        sum1=sum1+y
        sum2=sum2+y*y
    I=volume*sum1/n
    variance=(volume**2)*sum2/n-I*I
    dI=sqrt(variance/(n-1))
    return I,dI

def test_f_4d(x):
    return sin(x[0]+x[1]+x[2]+x[3])


def test_domain_4d(x):
    if x[0]**2+x[1]**2+x[2]**4+x[3]**2<1: return 1
    return 0

def identity(x):
    return 1

def test_mc_integrate_nd():
    g=MCG(time())
    N=10000
    box=[[0,1],[0,1],[0,1],[0,1]]
    volume=mc_integrate_nd(test_domain_4d,identity,1,N,g)
    print 'volume=',volume
    I=mc_integrate_nd(test_f_4d,box,test_domain_4d,volume[0],N,g)
    print 'integral=',I

# test_mc_integrate_nd()    
        
################################################################
# Network Simulation & Disjoint Sets
################################################################

class DisjSets:
    """
    The constructor of this class takes n (the number of nodes in an
    undirected graph) and     stores nodes connected by a path into equivalence classes.
    self.rep(i) returns the index of the representative element of the nodes connected to i
    rep(i)==rep(j) if and only if node i and node j are connected by     some path.
    The method is_path(x,y) returns 1 if there is a path connecting x and y,
    0 otherwise. Note that 0<=x,y<n.
    """
    def __init__(self,n):
        self.sets=range(n)
        for i in range(n):
            self.sets[i]=-1

    def rep(self,i):
        while self.sets[i]>=0:
            i=self.sets[i]
        return i

    def connect(self,i,j):
        i1=self.rep(i)
        j1=self.rep(j)
        if i1!=j1:
            self.sets[i1]=self.sets[i1]+self.sets[j1]
            self.sets[j1]=i1
                    
    def is_path(self,i,j):
        return self.rep(i)==self.rep(j)

class Network:
    """
    Network simulation program.
    n_nnodes (int) is the number of nodes of the network.
    links is a set of links (source, dest, p) where p is a
    probability that the link works.
    start is source of a signal and stop is the destiation of the signal.
    n is the number of signales to be simulated.
    The program computes the probability that a signal from start reaches stop.
    """
    def __init__(self,n_nodes,links,generator):
        self.n_nodes=n_nodes
        self.links=links
        self.generator=generator
    def simulate_once(self,start,stop):
        meta_net=DisjSets(self.n_nodes)
        for i,j,p in self.links:
            if self.generator.random()<p:
                meta_net.connect(i,j)
        if meta_net.is_path(start,stop):
            return 1
        return 0
    def simulate_many(self,start,stop,n):
        sum=0.0
        for i in range(n):
            sum=sum+self.simulate_once(start,stop)
        return sum/n

def test_Network():
    n_nodes=3
    links=[(0,1,0.5), (1,2,0.5)]
    generator=MCG()
    net=Network(n_nodes,links,generator)
    print net.simulate_many(0,2,1000)

# test_Network()    

################################################################
# Criticality of Nuclear Reactor
################################################################

class Queue:
    def __init__(self):
        self.queue=[]
        
    def enqueue(self,object):
        self.queue.append(deepcopy(object))

    def dequeue(self):
        if len(self.queue)==0:
            return None
        node=self.queue[0]
        del self.queue[0]
        return node	

   
class Point:
    def __init__(self,x=0,y=0,z=0):
        self.x=x
        self.y=y
        self.z=z

def domainSphere(p,r):
    # for example a sphere of radius one centered at the origin
    if sqrt(p.x**2+p.y**2+p.z**2)<r: return 1
    return 0

def randomPointInDomain(domain,r,g):
    p=Point()
    while true:
        p.x=r*(2.0*g.random()-1)
        p.y=r*(2.0*g.random()-1)
        p.z=r*(2.0*g.random()-1)
        if domain(p,r):
            return p
        
def randomDecayVector(lamb,g):
    p=Point()
    while 1:
        x=2.0*g.random()-1
        y=2.0*g.random()-1
        z=2.0*g.random()-1
        s2=x*x+y*y+z*z
        if s2<1:
            dist=g.exponential(lamb)/sqrt(s2)
            p.x=dist*x
            p.y=dist*y
            p.z=dist*z
            return p

class NuclearReactor:
    def __init__(self, domain, r, lamb, maxhits, generator):
        self.domain=domain
        self.r=r
        self.lamb=lamb
        self.maxhits=maxhits
        self.generator=generator
    def printVRML(self,filename):
        history=self.history
        radius=self.r
        file=open(filename,'w')
        VRMLInit(file)
        VRMLSphere(file,Point(0,0,0),radius,Color(0,1,1,0.8))
        for item in history:
            p=item[0]
            q=item[1]
            if item is history[0]:
                VRMLSphere(file,p,0.1,Color(1,0,0))
            VRMLSphere(file,q,0.05,Color(0,1,0))
            VRMLCylinder(file,p,q,0.01,Color(0,0,1))
        file.close()        
    def simulate_once(self):
        self.history=[]
        q=Queue()
        p=randomPointInDomain(self.domain,self.r,self.generator)
        while p:
            # first decay particle
            v=randomDecayVector(self.lamb,self.generator)
            u=Point(p.x+v.x,p.y+v.y,p.z+v.z)
            if self.domain(u,self.r):
                q.enqueue(u)
            # for plotting in vrml
            self.history.append(deepcopy([p,u]))
            # second decay particle
            v=randomDecayVector(self.lamb,self.generator)
            u=Point(p.x+v.x,p.y+v.y,p.z+v.z)
            if self.domain(u,self.r):
                q.enqueue(u)
            # for plotting in vrml
            self.history.append(deepcopy([p,u]))
            # if size of queue increases exponentially
            # we have a chain reaction
            if len(q.queue)>self.maxhits:                
                return 1
            p=q.dequeue()            
        # if size of queue went to zero
        # we have not chain reaction            
        return 0        

    def simulate_many(self,nevents):
        sum=0.0
        for i in range(nevents):
            sum=sum+self.simulate_once()
        return sum/nevents
    
def test_NuclearReactor():
    g=MCG()
    for r in range(5,15):
        reactor=NuclearReactor(domainSphere,r,lamb=0.1,maxhits=100,generator=g)
        print r, reactor.simulate_many(100)

# test_NuclearReactor()

################################################################
# Option Pricing
################################################################


class Transaction:
    def __init__(self,args):
	self.time=args[0]
        self.amount=args[1]

def PresentValueOfTransaction(transaction,r):	
    return transaction.amount*exp(-r*transaction.time)

def NetPresentValue(listTransactions,r):
    sum=0
    for item in listTransactions:
        sum=sum+PresentValueOfTransaction(Transaction(item),r)
    return sum

# print NetPresentValue([(0,100),(2,-50),(3,-40),(4,-30)],0.05)
	
def FutureCost(assetPrice,deliveryPrice,expirationTime,rate):	
    return assetPrice-deliveryPrice*exp(-rate*expirationTime)

def BinomialSimulation(S0,u,d,p,n):
    data=[]
    S=S0
    for i in range(n):
        data.append(S)
        if random()<p:
            S=u*S
        else:
            S=d*S
    return data

def MeasureMeanFromHistoric(historic, delta_t):    
    sum=0
    for i in range(len(historic)-1):
        sum=sum+(historic[i+1]-historic[i])/historic[i]
    return sum/(len(historic)-1)/delta_t

def MeasureVolatilityFromHistoric(historic, delta_t):
    sum=0
    for i in range(len(historic)-1):
        sum=sum+((historic[i+1]-historic[i])/historic[i])**2
    variance=(sum/(len(historic)-1)-(ComputeMean(historic)*delta_t)**2)
    volatility=sqrt(variance/delta_t)
    return volatility

def SimulateAsset(S0,mu,sigma,tau,delta_t,g):
    """
    SimulateAsset simulates the time evolution of the price of an asset
                  assuming a Ito Stochastic process:

        S(i+1)=S(i)*(1+mu*delta_t+sigma*g.gaussian()*sqrt(delta_t))

    S0      is the price today of the asset to simulate
    mu      is the expected yearly rate of return from the asset
    sigma   is yearly volatility of the asset
    tau     is the time of the simulation
    delta_t is the time step of the simulation
    g        is the random generator to be used
    """
    S=S0
    nsteps=int(tau/delta_t)
    for i in range(nsteps):
        S=S*(1+mu*delta_t+sigma*g.gaussian()*sqrt(delta_t))
    return S


def PriceOption(option,S0,mu,sigma,tau,delta_t,r,ap,g):
    """
    PriceOption simulates the time evolution of the price of an asset N
                times and compute the the current price of the option from

        C = exp(-r*tau)*intral(f(x)*p(x)*dx)
          = 1/N sum f(x_i)
          
    option  is the function that gives the value of the option
            at expiration as function of the stop price of the asset, x
    S0      is the price today of the asset to simulate
    mu      is the expected yearly rate of return from the asset
    sigma   is yearly volatility of the asset
    tau     is the time of the simulation
    delta_t is the time step of the simulation
    r       is the expected yearly interest on a Bank loan (continuous comp.)
    ap      is the required absolute precision of the simulation ($0.01)
    g       is the random generator to be used
    """
    sum=0.0
    sum2=0.0
    i=0
    while 1:
        i=i+1
        x_i=SimulateAsset(S0,mu,sigma,tau,delta_t,g)
        value_at_expiration=option(x_i)
        sum=sum+value_at_expiration
        sum2=sum2+value_at_expiration**2
        if i>100 and sqrt((sum2/i-(sum/i)**2)/i)<ap: break
    return exp(-r*tau)*sum/i

def LongCallOption(x,A=50):
    # Strike price of the option is 50$
    return max(x-A,0)

def ShortCallOption(x,A=50):
    # Strike price of the option is 50$
    return -max(x-A,0)

def LongPutOption(x,A=50):
    # Strike price of the option is 50$
    return max(A-x,0)

def ShortPutOption(x,A=50):
    # Strike price of the option is 50$
    return -max(A-x,0)


def StrangleOption(x):
    # this represnts a portflio of two options that is called a strangle
    return LongCallOption(x,A=60)+ShortPutOption(x,A=40)

def TabulatePriceOption(option, minS, maxS, stepS,
                        mu, sigma, tau, delta_t, r, ap, g):
    """
    TabulatePriceOption tabulates the option price for the current
        price of the underlying asset S in [minS, maxS] with incerements stepS
          
    option  is the function that gives the value of the option
            at expiration as function of the stop price of the asset, x
    minS    is the minimum current price of the asset to consider
    maxS    is the maximum current price of the asset to consider
    stepS   is the step to be used to compute S in range(minS,maxS,stepS)
    mu      is the expected yearly rate of return from the asset
    sigma   is yearly volatility of the asset
    tau     is the time of the simulation
    delta_t is the time step of the simulation
    ap      is the required absolute precision of the simulation ($0.01)
    g       is the random generator to be used
    """
    print 'asset price\toption price'
    for S in range(minS,maxS+stepS,stepS):
        print PriceOption(option,S,mu,sigma,tau,delta_t,r,ap,g)

def test_TabulatePriceOption():
    TabulatePriceOption(LongCallOption, #option to price
                    minS=30,maxS=70,stepS=2, # range of the simulation
                    mu=0.12,sigma=0.5,tau=50.0/253, # physical (measured) parameters
                    delta_t=1.0/253,r=0.05,ap=0.1,g=MCG()) # other parameters

#test_tabulatePriceOption()

################################################################
# Dependent variables and Metropolis
################################################################

class Metropolis:        
    def __init__(self,generator):
        self.generator=generator
        
    def step(self,p,q):
        old_x=self.x
        x=q(self.generator)
        if p(x)/p(old_x)<self.generator.random():
            x=old_x
        self.x=x

def exampleP(x):
    return 6.0*(x[0]-x[1])**2

def exampleQ(generator):
    x0=generator.random()
    x1=generator.random()
    return [x0, x1]

def test_Metropolis():
    m=Metropolis(MCG())
    m.x=exampleQ(m.generator)
    for i in range(100):
        m.step(exampleP,exampleQ)
        if i % 10 ==0 : print m.x
    
test_Metropolis()

def test_MetropolisIntegral(k=100,n=10000):
    m=Metropolis(MCG())
    m.x=exampleQ(m.generator)
    # termalize
    for i in range(n):
        m.step(exampleP,exampleQ)
    # average
    sum=0.0
    for i in range(n):
        m.step(exampleP,exampleQ)
        sum=sum+1.0/6.0*sin(m.x[0]*m.x[1])
        print sum/(i+1)

# test_MetropolisIntegral()

def test_PermutationDistance(n=10,generator=MCG()):
    x=range(n)
    sum=0.0
    counter=0
    while 1:
        counter=counter+1
        i=generator.randint(0,n-1)
        j=generator.randint(0,n-1)
        if i!=j:
            # swap i and j
            x[i],x[j] = x[j],x[i]
            sum2=0.0;
            for k in range(n+1):
                sum2=sum2+abs(x[i % n]-x[j % n])
            sum=sum+sum2/n
            print sum/counter

# test_PermutationDistance(10)

################################################################
# Ising Model
################################################################

class Ising:
    def __init__(self, n):
        s=[]        
        for i in range(n):
            s.append([])
            for j in range(n):
                s[i].append([])
                for k in range(n):
                    s[i][j].append(0)
                    s[i][j][k]=+1
        self.s=s
        self.n=n
        
    def E(self):
        n=self.n
        s=self.s
        sum=0.0
        for i in range(n):
            for j in range(n):
                for k in range(n):
                    sum=sum+s[i][j][k]*s[(i+1) % n][j][k]
                    sum=sum+s[i][j][k]*s[i][(j+1) % n][k]
                    sum=sum+s[i][j][k]*s[i][j][(k+1)%n]
        return sum

    def H(self):
        n=self.n
        s=self.s
        sum=0.0
        for i in range(n):
            for j in range(n):
                for k in range(n):
                    sum=sum+s[i][j][k]
        return sum/n**3

    def probability(self,t):
        return exp(-self.E()/t)

    def randomize(self,generator):
        n=self.n        
        for i in range(n):
            for j in range(n):
                for k in range(n):
                    if generator.random()<0.5:
                        self.s[i][j][k]=+1
                    else:
                        self.s[i][j][k]=-1

def test_IsingSimulate(n,t,g=MCG()):
    system=Ising(n)
    sum=0.0
    counter=0
    while 1:
        counter=counter+1
        old_system=deepcopy(system)
        system.randomize(g)
        if system.probability(t)/old_system.probability(t)<g.random():
            system=deepcopy(old_system)
        H=system.H()
        sum=sum+abs(H)
        print sum/counter

def test_IsingSimulate2(n,t,g=MCG()):
    system=Ising(n)
    sum=0.0
    counter=0
    s=system.s
    while 1:
        counter=counter+1
        i,j,k=g.randint(0,n-1), g.randint(0,n-1), g.randint(0,n-1)
        old_s=s[i][j][k]
        s[i][j][k]=-s[i][j][k]
        neighbours=s[(i+1) % n][j][k]+s[(i-1+n) % n][j][k]+s[i][(j+1) % n][k]+s[i][(j-1+n) % n][k]+s[i][j][(k+1)%n]+s[i][j][(k-1+n)%n]                   
        delta=-1.0/t*(s[i][j][k]-old_s)*neighbours
        if exp(delta)<g.random():
            s[i][j][k]=-s[i][j][k]
        H=system.H()
        sum=sum+abs(H)
        print sum/counter
        
test_IsingSimulate2(4,1)        

                    
################################################################
# Protein Folding (Simulated Annealing)
################################################################


class Protein:
    def __init__(self,amino):
        self.amino=amino
        
    def next_atom(self,p,move):
        if move==1:
            p=(p[0]+1,p[1],p[2])
        elif move==2:
            p=(p[0]-1,p[1],p[2])
        elif move==3:
            p=(p[0],p[1]+1,p[2])
        elif move==4:
            p=(p[0],p[1]-1,p[2])
        elif move==5:
            p=(p[0],p[1],p[2]+1)
        elif move==6:
            p=(p[0],p[1],p[2]-1)
        return p

    def energy(self):
        amino=self.amino
        folding=self.folding        
        n=len(amino)
        energy=0.0
        db={}
        p=(0,0,0)
        for i in range(len(amino)):
            if db.has_key(p):
                return None
            else:
                db[p]=amino[i]            
            if amino[i]=='H':
                r=(p[0]+1,p[1],p[2])
                if db.has_key(r):
                    if db[r]=='H':
                        energy=energy-1
                r=(p[0]-1,p[1],p[2])
                if db.has_key(r):
                    if db[r]=='H':
                        energy=energy-1
                r=(p[0],p[1]+1,p[2])
                if db.has_key(r):
                    if db[r]=='H':
                        energy=energy-1
                r=(p[0],p[1]-1,p[2])
                if db.has_key(r):
                    if db[r]=='H':
                        energy=energy-1
                r=(p[0],p[1],p[2]+1)
                if db.has_key(r):
                    if db[r]=='H':
                        energy=energy-1
                r=(p[0],p[1],p[2]-1)
                if db.has_key(r):
                    if db[r]=='H':
                        energy=energy-1
                if i>0 and amino[i-1]=='H':
                    energy=energy+1
            if i==n-1:
                break
            else:
                p=self.next_atom(p,folding[i])
        return energy

    def printVRML(self, filename):
        amino=self.amino
        folding=self.folding        
        file=open(filename,'w')
        VRMLInit(file)
        n=len(amino)
        p=(0,0,0)
        q=p
        db={}
        for a in range(len(amino)):
            p0=Point(p[0],p[1],p[2])
            if amino[a]=='P':
                VRMLSphere(file,p0,0.4,Color(0,1,0,0))
            else:
                VRMLSphere(file,p0,0.4,Color(1,0,0,0))
                db[p]=1
                i,j,k=p
                if db.has_key((i+1,j,k)):
                    q0=Point(i+1,j,k)
                    VRMLCylinder(file,p0,q0,0.1,Color(1,0,0,0.6))
                if db.has_key((i-1,j,k)):
                    q0=Point(i-1,j,k)
                    VRMLCylinder(file,p0,q0,0.1,Color(1,0,0,0.6))
                if db.has_key((i,j+1,k)):
                    q0=Point(i,j+1,k)
                    VRMLCylinder(file,p0,q0,0.1,Color(1,0,0,0.6))
                if db.has_key((i,j-1,k)):
                    q0=Point(i,j-1,k)
                    VRMLCylinder(file,p0,q0,0.1,Color(1,0,0,0.6))
                if db.has_key((i,j,k+1)):
                    q0=Point(i,j,k+1)
                    VRMLCylinder(file,p0,q0,0.1,Color(1,0,0,0.6))
                if db.has_key((i,j,k-1)):
                    q0=Point(i,j,k-1)
                    VRMLCylinder(file,p0,q0,0.1,Color(1,0,0,0.6))

            if a>0:
                r=Point(0.5*(p[0]+q[0]),0.5*(p[1]+q[1]),0.5*(p[2]+q[2]))
                VRMLSphere(file,r,0.2,Color(0,0,1,0))
            if a==n-1:
                break
            else:
                q=p
                p=self.next_atom(p,folding[a])
        file.write("}\n")
        file.close() 

    def folding(self, nsteps=300):
        amino=self.amino
        self.folding=folding=[]
        n=len(amino)
        counter=1
        for i in range(n-1):
            folding.append(1)
        old_energy=self.energy()
        sum_energy=old_energy
        min_energy=old_energy
        min_folding=folding
        while true:
            i=randint(1,n-2)
            j=randint(1,6)
            k=folding[i]
            folding[i]=j
            energy=self.energy()
            if energy != None:
                if exp(-energy)/exp(-old_energy)>random():
                    old_energy=energy
                else:
                    folding[i]=k
                    energy=old_energy
                print counter, sum_energy/counter, min_energy
     
                sum_energy=sum_energy+energy
                counter=counter+1                
     
                if energy<min_energy:
                    min_energy=energy
                    min_folding=copy(folding)
            else:
                folding[i]=k        
            if counter==nsteps:
                break
        return sum_energy/counter, min_energy, min_folding

def test_folding(amino="HHPHPHHPHPHPPPPHHPHPHHPHPPHHHHP"):
    protein=Protein(amino)
    avg,min,folding=protein.folding(100)
    protein.printVRML("folding.wrl")

#test_folding()

def menu():
    #test_reactor(5,10)
    #test_shielding(0,10)
    #test_network()
    #test_folding()
    #IsingGreen(6)



