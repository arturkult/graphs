import simplegraphs
import inspect
try:
    graph = simplegraphs.SortedEdgesList("I??????w?")
    #graph.deleteEdge(1,9)
    #print(str(graph))
    #graph.addEdge(0,8)
    graph.addVertex()
    print(str(graph))
    graph.deleteVertex(graph.order()-1)
    print(str(graph))
    print(graph.isEdge(0, 9))
    print(graph.isEdge(1, 9))
    print(graph.isEdge(2, 9))
except simplegraphs.SortedEdgesList.G6Error as e:
    print(e);
# print(inspect.getmro(SortedEdgesList))
# print([x for x in dir(simplegraphs ) if not x.startswith( "_" )])
# print([x for x in dir( Graph ) if not x.startswith( "_" )])