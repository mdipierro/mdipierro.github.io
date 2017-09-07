import urllib,re

def download_papers():
    page = urllib.urlopen('http://www.slac.stanford.edu/spires/find/hep/www?rawcmd=FIND+A+MASSIMO+DI+PIERRO&FORMAT=wwwbrief&SEQUENCE=').read()
    print page
    items = re.compile('\[arXiv\:(.*?)\]').findall(page)
    for item in items:
        print item
        pdf = urllib.urlopen('http://arxiv.org/pdf/'+item).read()
        open(item.replace('/','.')+'.pdf','wb').write(pdf)
    items = re.compile('\[(hep\-.*?)\]').findall(page)
    for item in items:
        print item
        pdf = urllib.urlopen('http://arxiv.org/pdf/'+item).read()
        open(item.replace('/','.')+'.pdf','wb').write(pdf)
    items = re.compile('\[(cs/.*?)\]').findall(page)
    for item in items:
        print item
        pdf = urllib.urlopen('http://arxiv.org/pdf/'+item).read()
        open(item.replace('/','.')+'.pdf','wb').write(pdf)


