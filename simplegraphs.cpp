#include <iostream>
#include <Python.h>
#include <structmember.h>
#include <vector>
#include <algorithm>

using namespace std;
#define MAX_VERTICES 16
#define MAX_EDGES MAX_VERTICES * (MAX_VERTICES - 1) / 2
#define ORDER_INDEX 0

typedef struct {
	PyObject_HEAD int start;
	int end;
}Edge;

typedef struct {
	PyObject_HEAD size_t order;
	vector<Edge> list;
	const char* text;
} SortedEdgesList;

static PyObject* G6Error;
static PyObject* NoVerticesError;
static PyObject* TooManyVerticesError;

static void SortedEdgesList_dealloc(SortedEdgesList *self) {
	Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *SortedEdgesList_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	SortedEdgesList *self;

	self = (SortedEdgesList *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->order = 0;
	}

	return (PyObject *)self;
}

static PyObject* SortedEdgesList_order(SortedEdgesList* self) {
	return PyLong_FromSize_t(self->order);
}

static bool compare(Edge o1, Edge o2) {
	if (o1.start < o2.start) {
		return true;
	}
	if (o1.start > o2.start) {
		return false;
	}
	if (o1.end < o2.end) {
		return true;
	}
	return false;
}

static PyObject* SortedEdgesList_addEdge(SortedEdgesList *self, PyObject *args, PyObject *kwds) {
	PyObject *res;
	int start, end;
	PyArg_ParseTuple(args, "ii", &start, &end);
	Edge e = Edge();
	e.start = start < end ? start : end;
	e.end = end > start ? end : start;
	bool wasInterrupted = false;
	for (auto it = self->list.begin(); it != self->list.end(); it++) {
		if ((*it).start == e.start && (*it).end == e.end) { // edge already excists
			wasInterrupted = true;
			break;
		}
		if ((*it).start > e.start
			||
			((*it).start == e.start && (*it).end > e.end)) {
			self->list.emplace(it, e);
			wasInterrupted = true;
			break;
		}
	}
	if (!wasInterrupted) {
		self->list.push_back(e);
	}
	res = Py_None;
	Py_INCREF(res);
	return res;
}

static PyObject* SortedEdgesList_deleteEdge(SortedEdgesList *self, PyObject *args, PyObject *kwds) {
	PyObject *res;
	int start, end;
	PyArg_ParseTuple(args, "ii", &start, &end);
	Edge e = Edge();
	e.start = start < end ? start : end;
	e.end = end > start ? end : start;
	for (auto it = self->list.begin(); it != self->list.end(); it++) {		
		if ((*it).start == e.start
			&& (*it).end == e.end) {
			self->list.erase(it);
			break;
		}
	}
	res = Py_None;
	Py_INCREF(res);
	return res;
}

static PyObject* SortedEdgesList_fromString(SortedEdgesList *self, const char* text = "@") {
	PyObject* res;
	size_t len = strlen(text);
	self->text = text;
	int c = (int)text[ORDER_INDEX] - 63; // ?
	if (c < 1 || c > 16) {
		return PyErr_Format(G6Error, "wrong order : %d", c);

	}
	self->order = c;
	int k = 0, t = 1;
	for (int i = 1; i < self->order; i++) {
		for (int j = 0; j < i; j++) {
			if (t > strlen(text))
			{
				return PyErr_Format(G6Error, "too short text");
			}
			if (k == 0) {
				c = (int)text[t++] - 63;
				if (c < 0 || c > 63) {
					return PyErr_Format(G6Error, "wrong character:");

				}
				k = 6;
			}
			k -= 1;
			if ((c & (1 << k)) != 0) {
				PyObject* args = Py_BuildValue("ii", i, j);
				SortedEdgesList_addEdge(self, args, NULL);
				Py_DECREF(args);
			}
		}
	}
	if (t < strlen(text))
	{
		return PyErr_Format(G6Error, "too long text");
	}
	res = Py_None;
	Py_INCREF(res);
	return res;
}

static PyObject* SortedEdgesList_isEdge(SortedEdgesList *self, PyObject *args, PyObject *kwds) {
	PyObject* res;
	int start, end;
	PyArg_ParseTuple(args, "ii", &start, &end);
	Edge e = Edge();
	e.start = start < end ? start : end;
	e.end = end > start ? end : start;
	for (auto it = self->list.begin(); it != self->list.end(); it++) {
		if ((*it).start == e.start && (*it).end == e.end) {
			res = Py_True;
			Py_INCREF(res);
			return res;
		}
	}
	//printf("NE\n");
	res = Py_False;
	Py_INCREF(res);
	return res;
}

static PyObject* SortedEdgesList_addVertex(SortedEdgesList *self) {
	PyObject* res;
	if (self->order == MAX_VERTICES) {
		PyErr_SetString(TooManyVerticesError, "too many vertices");
		return NULL;
	}
	self->order += 1;
	res = Py_None;
	Py_INCREF(res);
	return res;
}

static PyObject* SortedEdgesList_deleteVertex(SortedEdgesList *self, PyObject* args) {
	PyObject* res;
	int vertex;
	PyArg_ParseTuple(args, "i", &vertex);
	if (self->order == 1) {
		PyErr_SetString(NoVerticesError, "graph must have vertices");
		return NULL;
	}
	for (auto it = self->list.begin(); it != self->list.end(); it++) {
		if ((*it).start == vertex || (*it).end == vertex) {
			PyObject* args = Py_BuildValue("ii", (*it).start, (*it).end);
			SortedEdgesList_deleteEdge(self, args, NULL);
			Py_DECREF(args);
		}

	}
	self->order -= 1;
	res = Py_None;
	Py_INCREF(res);
	return res;
}

static PyObject* SortedEdgesList_str(SortedEdgesList *self) {
	char text[15] = "";
	int currIndex = 0;
	text[currIndex++] = (char)(self->order + 63);
	int k = 5, c = 0;
	for (int i = 1; i < self->order; i++) {
		for (int j = 0; j < i; j++) {
			PyObject* args = Py_BuildValue("ii", i, j);
			if (PyObject_IsTrue(SortedEdgesList_isEdge(self, args, NULL))) {
				c |= (1 << k);
			}
			Py_DECREF(args);
			if (k == 0) {
				text[currIndex++] = char(c + 63);
				k = 6;
				c = 0;
			}
			k -= 1;
		}
	}
	if (k != 5) {
		text[currIndex++] = char(c + 63);
	}

	return Py_BuildValue("s", text);
}


static int SortedEdgesList_init(SortedEdgesList *self, PyObject* args, PyObject *kwds) {
	const char* text = "@";
	PyArg_ParseTuple(args, "|s", &text);
	PyObject* result = SortedEdgesList_fromString(self, text);
	if (result == NULL)
	{
		return -1;
	}
	else return 0;
}


static PyMethodDef SortedEdgesList_methods[] = {
	{"order", (PyCFunction)SortedEdgesList_order, METH_NOARGS, "gets order"},
	{"isEdge", (PyCFunction)SortedEdgesList_isEdge, METH_VARARGS, "check is edge excists"},
	{"addEdge", (PyCFunction)SortedEdgesList_addEdge, METH_VARARGS, "insert edge to list"},
	{"deleteEdge", (PyCFunction)SortedEdgesList_deleteEdge, METH_VARARGS, "delete edge from list"},
	{"addVertex", (PyCFunction)SortedEdgesList_addVertex, METH_NOARGS, "delete vertex to graph"},
	{"deleteVertex", (PyCFunction)SortedEdgesList_deleteVertex, METH_VARARGS, "delete vertex from graph"},
	{"fromString", (PyCFunction)SortedEdgesList_fromString, METH_O, "parse graph from string"},
	{NULL}
};

static PyMemberDef SortedEdgesList_members[] = {
	{"G6Error", T_OBJECT, sizeof(G6Error), 0, "Exception while parsing graph"},
	{"__order", T_INT, offsetof(SortedEdgesList, order), 0, "order of graph"},
	{"__list", T_INT, offsetof(SortedEdgesList, list), 0, "sorted order list"},
	{"__text", T_INT, offsetof(SortedEdgesList, text), 0, "string of graph"},
	{NULL}
};

static PyTypeObject SortedEdgesListTypeObj = {
	PyVarObject_HEAD_INIT(NULL, 0) "simplegraphs.SortedEdgesList",  /* tp_name */
	sizeof(SortedEdgesList),                           /* tp_basicsize */
	0,                                        /* tp_itemsize */
	(destructor)SortedEdgesList_dealloc,               /* tp_dealloc */
	0,                                        /* tp_print */
	0,                                        /* tp_getattr */
	0,                                        /* tp_setattr */
	0,                                        /* tp_reserved */
	0,                                        /* tp_repr */
	0,                                        /* tp_as_number */
	0,                                        /* tp_as_sequence */
	0,                                        /* tp_as_mapping */
	(hashfunc)_Py_HashPointer,                                        /* tp_hash  */
	0,                                        /* tp_call */
	(reprfunc)SortedEdgesList_str,                                        /* tp_str */
	0,                                        /* tp_getattro */
	0,                                        /* tp_setattro */
	0,                                        /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"SortedEdgesList",                         /* tp_doc */
	0,                                        /* tp_traverse */
	0,                                        /* tp_clear */
	0,									      /* tp_richcompare */
	0,                                        /* tp_weaklistoffset */
	0,                                        /* tp_iter */
	0,                                        /* tp_iternext */
	SortedEdgesList_methods,                           /* tp_methods */
	SortedEdgesList_members,                           /* tp_members */
	0,                                        /* tp_getset */
	0,                                        /* tp_base */
	0,                                        /* tp_dict */
	0,                                        /* tp_descr_get */
	0,                                        /* tp_descr_set */
	0,                                        /* tp_dictoffset */
	(initproc)SortedEdgesList_init,                    /* tp_init */
	0,                                        /* tp_alloc */
	SortedEdgesList_new,                               /* tp_new */
};

PyMODINIT_FUNC PyInit_simplegraphs() {
	PyObject *module;
	Py_Initialize();
	static struct PyModuleDef moduledef = {
		PyModuleDef_HEAD_INIT,
		"simplegraphs",
		"Simple graphs module",
		0
	};

	module = PyModule_Create(&moduledef);

	if (PyType_Ready(&SortedEdgesListTypeObj) < 0)
		return NULL;
	Py_INCREF(&SortedEdgesListTypeObj);
	PyModule_AddObject(module, "SortedEdgesList", (PyObject *)&SortedEdgesListTypeObj);

	G6Error = PyErr_NewException("simplegraphs.G6Error", NULL, NULL);
	Py_INCREF(G6Error);
	PyDict_SetItemString(SortedEdgesListTypeObj.tp_dict, "G6Error", G6Error);

	NoVerticesError = PyErr_NewException("simplegraphs.SortedEdgesList.NoVerticesError", NULL, NULL);
	Py_INCREF(NoVerticesError);
	PyDict_SetItemString(SortedEdgesListTypeObj.tp_dict, "NoVerticesError", NoVerticesError);

	TooManyVerticesError = PyErr_NewException("simplegraphs.SortedEdgesList.TooManyVerticesError", NULL, NULL);
	Py_INCREF(TooManyVerticesError);
	PyDict_SetItemString(SortedEdgesListTypeObj.tp_dict, "TooManyVerticesError", TooManyVerticesError);

	return module;
}