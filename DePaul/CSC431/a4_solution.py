from numeric import *
import datetime, time
from math import *

N=100 # number of calls for timing

def E(f,data):
    """ compute expectation value """
    return sum(f(x) for x in data)/len(data)
def N1(z):
    """
    exp(-z^2) cdf from 
    http://finance.bi.no/~bernt/gcc_prog/recipes/recipes/node23.html
    """
    if z<0: return 1.0-N1(-z)
    elif z>+6: return 1
    v = [0.31938153,-0.356563782,1.781477937,-1.82155978,1.330273329]
    t = 1.0/(1.0+0.2316419*abs(z))
    ret= 1.0-0.3989423*exp(-z*z/2)*((((v[4]*t+v[3])*t+v[2])*t+v[1])*t+v[0])*t
    print z, ret
    return ret
def N2(z):
    """
    exp(-z^2) cdf, numerical integration, problem 4.1
    """
    if z<0: return 1.0-N2(-z)
    elif z>6: return 1
    else:
        f = lambda z: exp(-z*z)
        return 0.5+integrate_quadrature_naive(f,0,z/sqrt(2))/sqrt(pi)

def erf(x,ap=1e-5,ns=100):
    """
    the error function from polynomial approximation (problem 4.2)
    """
    term = 2.0*x/sqrt(pi)
    result = term    
    for k in range(1,ns):        
        term = term*(-x*x/k)*(2.0*k-1)/(2.0*k+1)
        result += term
        if abs(term)<ap: return result
def N3(z):
    """
    exp(-z^2) cdf using erf, problem 4.2
    """
    return 0.5+0.5*erf(z/sqrt(2))

def PriceCall(S,X,r,sigma,t,N=N1):
    """ black sholes pricing """
    d1 = (log(S/X)+r*t)/(sigma*sqrt(t)) + 0.5*sigma*sqrt(t)
    d2 = d1 - sigma*sqrt(t)
    return S*N(d1)-X*exp(-r*t)*N(d2)

def solve_problem():
    aapl2008 = YStock('AAPL').historical(start=datetime.date(2008,1,1),stop=datetime.date(2009,1,1))
    data = [day['log_return'] for day in aapl2008[1:]]
    mu = E(lambda x: x, data)
    sigma = sqrt(E(lambda x: (x-mu)**2, data))
    S = aapl2008[-1]['adjusted_close']
    print S
    X = 150.0
    t = 2.0*len(data) # trading days in year
    r = 0.03/len(data) # daily risk free return
    t0 = time.time()
    for k in range(N):
        r1 = PriceCall(S,X,r,sigma,t,N=N1)
    t0,t1 = time.time(), (time.time()-t0)/N
    for k in range(N):
        r2 = PriceCall(S,X,r,sigma,t,N=N2)
    t0,t2 = time.time(), (time.time()-t0)/N
    for k in range(N):
        r3 =PriceCall(S,X,r,sigma,t,N=N3)
    t0,t3 = time.time(), (time.time()-t0)/N
    print "option price, best solution",r1,t1
    print "option price, using numerical integration",r2,t2
    print "option price, using polynomail approx for error function",r3,t3

solve_problem()
"""
output:
price time(secs)
option price, best solution 
14.5174890434 0.000123121738434
option price, using numerical integration 
14.5217013864 0.00147810935974
option price, using polynomail approx for error function
14.5217079876 1.36089324951e-05
"""
