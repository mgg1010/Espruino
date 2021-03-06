/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * Header for auto-generated Wrapper functions
 * ----------------------------------------------------------------------------
 */

#ifndef JSWRAPPER_H
#define JSWRAPPER_H

#include "jsutils.h"
#include "jsparse.h"

typedef enum {
  JSWAT_FINISH = 0, // no argument
  JSWAT_VOID = 0, // Only for return values
  JSWAT_JSVAR, // standard variable
  JSWAT_ARGUMENT_ARRAY, // a JsVar array containing all subsequent arguments
  JSWAT_BOOL, // boolean
  JSWAT_INT32, // 32 bit int
  JSWAT_PIN, // A pin
  JSWAT_JSVARINT, // 64 bit int
  JSWAT_JSVARFLOAT, // 64 bit float
  JSWAT__LAST = JSWAT_JSVARFLOAT,
  JSWAT_MASK = NEXT_POWER_2(JSWAT__LAST)-1,

  JSWAT_THIS_ARG = 0x8000000
} JsnArgumentType;

// number of bits needed for each argument bit
#define JSWAT_BITS GET_BIT_NUMBER(JSWAT_MASK+1)

/** If 'name' is something that belongs to an internal function, execute it.  */
JsVar *jswFindBuiltInFunction(JsVar *parent, const char *name);

/// Given the name of an Object, see if we should set it up as a builtin or not
bool jswIsBuiltInObject(const char *name);

/// If we get this in 'require', should we make an object with this name?
bool jswIsBuiltInLibrary(const char *name);

/** Given a variable, return the basic object name of it */
const char *jswGetBasicObjectName(JsVar *var);

/** Given the name of a Basic Object, eg, Uint8Array, String, etc. Return the prototype object's name - or 0.
 * For instance jswGetBasicObjectPrototypeName("Object")==0, jswGetBasicObjectPrototypeName("Integer")=="Object",
 * jswGetBasicObjectPrototypeName("Uint8Array")=="ArrayBufferView"
 *  */
const char *jswGetBasicObjectPrototypeName(const char *name);

/** Tasks to run on Idle. Returns true if either one of the tasks returned true (eg. they're doing something and want to avoid sleeping) */
bool jswIdle();

/** Tasks to run on Initialisation */
void jswInit();

/** Tasks to run on Deinitialisation */
void jswKill();

#endif // JSWRAPPER_H
