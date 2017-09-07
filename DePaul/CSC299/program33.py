import csv
myDict = {}

with open('expenses.csv', 'rb') as csvfile:
    ExpensesReader = csv.reader(csvfile)
    for row in ExpensesReader:
       if  row[0] == "USER_ID":
           pass
       else:
           if row[0] not in myDict: 
               myDict[row[0]] = int(row[1].strip("$"))
           else:
               myDict[row[0]] += int(row[1].strip("$"))

with open('totals.csv', 'wb') as csvfile:
   TotalReader = csv.writer(csvfile)
   TotalReader.writerow(['USER_ID','TOTAL EXPENSE'])
   keylist = myDict.keys()
   keylist.sort()
   for key in keylist:
       line = [key, '$%s' % myDict[key]]
       TotalReader.writerow(line)
                   
