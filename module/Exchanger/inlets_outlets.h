// -*- C++ -*-
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  <LicenseText>
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//

#if !defined(pyExchanger_inlets_outlets_h)
#define pyExchanger_inlets_outlets_h


///////////////////////////////////////////////////////////////////////////////


extern char pyExchanger_Inlet_impose__name__[];
extern char pyExchanger_Inlet_impose__doc__[];
extern "C"
PyObject * pyExchanger_Inlet_impose(PyObject *, PyObject *);


extern char pyExchanger_Inlet_recv__name__[];
extern char pyExchanger_Inlet_recv__doc__[];
extern "C"
PyObject * pyExchanger_Inlet_recv(PyObject *, PyObject *);


extern char pyExchanger_Inlet_storeTimestep__name__[];
extern char pyExchanger_Inlet_storeTimestep__doc__[];
extern "C"
PyObject * pyExchanger_Inlet_storeTimestep(PyObject *, PyObject *);


///////////////////////////////////////////////////////////////////////////////


extern char pyExchanger_Outlet_send__name__[];
extern char pyExchanger_Outlet_send__doc__[];
extern "C"
PyObject * pyExchanger_Outlet_send(PyObject *, PyObject *);


///////////////////////////////////////////////////////////////////////////////


extern char pyExchanger_BoundaryVTInlet_create__name__[];
extern char pyExchanger_BoundaryVTInlet_create__doc__[];
extern "C"
PyObject * pyExchanger_BoundaryVTInlet_create(PyObject *, PyObject *);


extern char pyExchanger_VTInlet_create__name__[];
extern char pyExchanger_VTInlet_create__doc__[];
extern "C"
PyObject * pyExchanger_VTInlet_create(PyObject *, PyObject *);


///////////////////////////////////////////////////////////////////////////////


extern char pyExchanger_VTOutlet_create__name__[];
extern char pyExchanger_VTOutlet_create__doc__[];
extern "C"
PyObject * pyExchanger_VTOutlet_create(PyObject *, PyObject *);


#endif

// version
// $Id: inlets_outlets.h,v 1.3 2004/03/11 23:23:49 tan2 Exp $

// End of file
