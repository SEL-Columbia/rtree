/*
# =============================================================================
# Rtree spatial index. Copyright (C) 2007 Sean C. Gillies
#
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 2.1 of the License, or (at your option)
# any later version.
#
# This library is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License 
# along with this library; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# Contact email: sgillies@frii.com
# =============================================================================
*/

#include "gispyspatialindex.h"
#include "Python.h"

#include "idx_config.h"

#include <stack>
#include <string>

static std::stack<std::string> errors;

using namespace SpatialIndex;

class PyListVisitor : public IVisitor
{
public:
    PyListVisitor(PyObject *o)
    {
        ids = o;
        Py_INCREF(ids);
    }
    
    ~PyListVisitor()
    {
        Py_DECREF(ids);
    }
    
    void visitNode(const INode & n) {}
    
    void visitData(const IData & d)
    {
        PyObject* ob = PyLong_FromLongLong(d.getIdentifier()); 
        PyList_Append(ids, ob); 
        Py_DECREF(ob); 
    }

    void visitData(std::vector<const IData*>& v) {}

private:
    PyObject *ids;
};

IDX_C_START

IndexH Index_Create(const char* pszFilename, IndexProperties* properties)
{
    return (IndexH) new GISPySpatialIndex();    
}

void Index_Delete(IndexH index)
{
    GISPySpatialIndex* idx = (GISPySpatialIndex*) index;
    if (idx) delete idx;
}

RTError Index_DeleteData(IndexH index, uint64_t id, double* pdMin, double* pdMax, uint32_t nDimension)
{
    GISPySpatialIndex* idx = (GISPySpatialIndex*) index;
    
    try {	
        idx->index().deleteData(SpatialIndex::Region(pdMin, pdMax, nDimension), id);
        return RT_None;
    }
    catch (Tools::Exception& e) {
        // PyErr_SetString(PyExc_TypeError, e.what().c_str());
        return RT_Fatal;
    }
}
IDX_C_END

extern "C"
GISPySpatialIndex *
RtreeIndex_new(char* filename, uint32_t nPageLength, int load)
{
    if (!filename)
        return new GISPySpatialIndex;
    else
    {
        if (load == 1)
        {
            return new GISPySpatialIndex(filename);
        }
        else
        {
            if (!nPageLength) nPageLength=4096;
            return new GISPySpatialIndex(filename, nPageLength);
        }
    }
}

extern "C"
void
RtreeIndex_del(GISPySpatialIndex *index)
{
    delete index;
}

extern "C"
int
RtreeIndex_insertData(GISPySpatialIndex *index, uint64_t id,
                      double *min, double *max)
{
    try {	
        index->index().insertData(0, 0, SpatialIndex::Region(min, max, 2), id);
        return 1;
    }
    catch (Tools::Exception& e) {
        PyErr_SetString(PyExc_TypeError, e.what().c_str());
        return 0;
    }
}

extern "C"
int
RtreeIndex_deleteData(GISPySpatialIndex *index, uint64_t id,
                      double *min, double *max)
{
    try {	
        index->index().deleteData(SpatialIndex::Region(min, max, 2), id);
        return 1;
    }
    catch (Tools::Exception& e) {
        PyErr_SetString(PyExc_TypeError, e.what().c_str());
        return NULL;
    }
  
}

extern "C"
PyObject *
RtreeIndex_intersects(GISPySpatialIndex *index, double *min, double *max)
{
    /* get intersecting data */
    int count=0;
    PyObject *ids;

    ids = PyList_New((size_t)count);
    PyListVisitor *visitor = new PyListVisitor(ids);

    try {	
        const SpatialIndex::Region *region = new SpatialIndex::Region(min, max, 2);
        index->index().intersectsWithQuery((*region), (*visitor));
        delete region;
        delete visitor;
        return ids;
    }
    catch (Tools::Exception& e) {
        PyErr_SetString(PyExc_TypeError, e.what().c_str());
        delete visitor;
        return NULL;
    }

}

extern "C"
int
RtreeIndex_isValid(GISPySpatialIndex *index)
{
  try {	
      return (int) index->index().isIndexValid();
  }
  catch (...) {
     // isIndexValid throws an exception for empty indexes which we'll assume is valid
	return 1; 
  }
}

extern "C"
PyObject *
RtreeIndex_nearestNeighbors(GISPySpatialIndex *index, uint32_t num_results, double *min, double *max)
{
    /* get intersecting data */
    int count=0;
    PyObject *ids;

    ids = PyList_New((size_t)count);
    PyListVisitor *visitor = new PyListVisitor(ids);
    try {	
        const SpatialIndex::Region *region = new SpatialIndex::Region(min, max, 2);
        index->index().nearestNeighborQuery(num_results, (*region), (*visitor));
        delete region;
        delete visitor;
        return ids;
    }
    catch (Tools::Exception& e) {
        PyErr_SetString(PyExc_TypeError, e.what().c_str());
        delete visitor;
        return NULL;
    }
    
}

