/*
 * cubieboard emulation
 *
 * Copyright (C) 2013 Li Guang
 * Written by Li Guang <lig.fnst@cn.fujitsu.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "hw/boards.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdlib.h>

extern PyObject *PyInit__pyboard(void);
extern PyObject *MachineClass2Py(MachineClass *mc);

/* load "pyboard" script (from env vairable for now) 
 *
 * returns: The module PyObject
 */
PyObject * load_python_script(void)
{
    char *pyboard_script = getenv( "QEMU_PYBOARD_SCRIPT" );
    if(pyboard_script == NULL) {
        warn_report("$QEMU_PYBOARD_SCRIPT is empty. Use this env variable to specify your python script in $PYTHONPATH.");
        return NULL;
    }

    PyObject *pName, *pModule;

    PyImport_AppendInittab("_pyboard", &PyInit__pyboard);
    Py_Initialize();
    pName = PyUnicode_DecodeFSDefault(pyboard_script);
    /* Error checking of pName left out */
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if(!pModule){
        error_report("PyImport_Import on script %s failed. Check PYTHONPATH\n", pyboard_script);
        exit(1);
    }

    return pModule; 
}

static void pyboard_machine_init(MachineClass *mc)
{
    PyObject *pModule, *pFunc;

    pModule = load_python_script();
        if( pModule != NULL ) {

        pFunc = PyObject_GetAttrString(pModule, "machine_init");
        if(!pFunc){
            error_report("PyObject_GetAttrString failed");
            exit(1);
        }

        PyObject *pret = PyObject_CallFunction(pFunc, "(O)", MachineClass2Py(mc));
        if(!pret){
            error_report("PyObject_CallFunction failed");
            exit(1);
        }
        Py_DECREF(pret);
    }
}

DEFINE_MACHINE("pyboard", pyboard_machine_init)
