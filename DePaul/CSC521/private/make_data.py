import csv
import random

t = 0
f = open('accidents.csv','w')
while t < 4*365:
      t = t + random.expovariate(0.2)
      plant = random.choice(('A','B'))
      loss = 10**random.gauss(4 if plant=='A' else 3, 0.5)
      f.write('%s,%i,%i\n' % (plant, int(t), int(loss)))
