import random

Infinity = float("inf")

def random_matrix(n=5):
    adj = [[0]*n for k in range(n)]
    for i in range(n):
        for j in range(i):
            adj[i][j]=adj[j][i] = random.randint(1,10)
    return adj

class TSP(object):

    def __init__(self,links):
        self.links = links
        self.n = len(links)
        self.vertices = range(self.n)

    def bound(self,path):
        length = 0
        path_length = len(path)
        for k in range(path_length):
            i,j = path[k],path[(k+1)%path_length]
            length += self.links[i][j]
        return length

    def solve(self):
        Q = [[0]]
        min_weight = Infinity
        while Q:
            parent_path = Q.pop()
            for vertex in self.vertices:
                if not vertex in parent_path:
                    child_path = parent_path+[vertex]
                    bound = self.bound(child_path)
                    print child_path, '->', bound
                    if len(child_path) == self.n and bound<min_weight:
                        min_weight, solution = bound, child_path
                    elif bound<min_weight:
                        Q = Q + [child_path]
        return min_weight, solution

links = random_matrix()
print TSP(links).solve()
