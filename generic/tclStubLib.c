/*
 * tclStubLib.c --
 *
 *	Stub object that will be statically linked into extensions that want
 *	to access Tcl.
 *
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 * Copyright (c) 1998 Paul Duffin.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

MODULE_SCOPE const TclStubs *tclStubsPtr;
MODULE_SCOPE const TclPlatStubs *tclPlatStubsPtr;
MODULE_SCOPE const TclIntStubs *tclIntStubsPtr;
MODULE_SCOPE const TclIntPlatStubs *tclIntPlatStubsPtr;

const TclStubs *tclStubsPtr = NULL;
const TclPlatStubs *tclPlatStubsPtr = NULL;
const TclIntStubs *tclIntStubsPtr = NULL;
const TclIntPlatStubs *tclIntPlatStubsPtr = NULL;

/*
 * Use our own ISDIGIT to avoid linking to libc on windows
 */

#define ISDIGIT(c) (((unsigned)((c)-'0')) <= 9)

/*
 *----------------------------------------------------------------------
 *
 * TclInitStubs --
 *
 *	Tries to initialise the stub table pointers and ensures that the
 *	correct version of Tcl is loaded.
 *
 * Results:
 *	The actual version of Tcl that satisfies the request, or NULL to
 *	indicate that an error occurred.
 *
 * Side effects:
 *	Sets the stub table pointers.
 *
 *----------------------------------------------------------------------
 */
MODULE_SCOPE const char *
TclInitStubs(
    Tcl_Interp *interp,
    const char *version,
    int exact,
    const char *tclversion,
    int magic)
{
    Interp *iPtr = (Interp *) interp;
    const char *actualVersion = NULL;
    ClientData pkgData = NULL;
    const TclStubs *stubsPtr = iPtr->stubTable;

    /*
     * We can't optimize this check by caching tclStubsPtr because that
     * prevents apps from being able to load/unload Tcl dynamically multiple
     * times. [Bug 615304]
     */

    if (!stubsPtr || (stubsPtr->magic != TCL_STUB_MAGIC)) {
	/* This can only be executed in a Tcl < 8.1 interpreter, because
	 * the magic values are kept the same in later versions. */
	iPtr->objResultPtr = (Tcl_Obj *)
		"interpreter uses an incompatible stubs mechanism";
	iPtr->emptyObjPtr = 0; /* TCL_STATIC */
	return NULL;
    }

    actualVersion = stubsPtr->tcl_PkgRequireEx(interp, "Tcl", version, 0, &pkgData);
    if (actualVersion == NULL) {
	return NULL;
    }
    if (exact&1) {
	const char *p = version;
	int count = 0;

	while (*p) {
	    count += !ISDIGIT(*p++);
	}
	if (count == 1) {
	    const char *q = actualVersion;

	    p = version;
	    while (*p && (*p == *q)) {
		p++; q++;
	    }
	    if (*p || ISDIGIT(*q)) {
		/* Construct error message */
		stubsPtr->tcl_PkgRequireEx(interp, "Tcl", version, 1, NULL);
		return NULL;
	    }
	} else {
	    actualVersion = stubsPtr->tcl_PkgRequireEx(interp, "Tcl", version, 1, NULL);
	    if (actualVersion == NULL) {
		return NULL;
	    }
	}
    }

#define MASK (4+8+16) /* possible values of sizeof(size_t) */

    /* reserved77 is the location of Tcl_Backslash, which was removed
     * in Tcl 9.0. If this value is NULL, we know that we have Tcl > 8
     */
    if ((exact & MASK) != (int)
	    ((stubsPtr->reserved77)?sizeof(int):sizeof(size_t))) {
	char *msg = stubsPtr->tcl_Alloc(64 + strlen(tclversion) + strlen(version));

	strcpy(msg, "incompatible stub library: have ");
	strcat(msg, tclversion);
	strcat(msg, ", need ");
	if (stubsPtr->reserved77) {
	    /* Take "version", but strip off everything after '-' */
	    char *p = msg + strlen(msg);
	    while (*version && *version != '-') {
		*p++ = *version++;
	    }
	    *p = '\0';
	} else {
	    strcat(msg, "9");
	}
	stubsPtr->tcl_SetObjResult(interp, stubsPtr->tcl_NewStringObj(msg, -1));
	stubsPtr->tcl_Free(msg);
	return NULL;
    }
    tclStubsPtr = (TclStubs *)pkgData;

    if (tclStubsPtr->hooks) {
	tclPlatStubsPtr = tclStubsPtr->hooks->tclPlatStubs;
	tclIntStubsPtr = tclStubsPtr->hooks->tclIntStubs;
	tclIntPlatStubsPtr = tclStubsPtr->hooks->tclIntPlatStubs;
    } else {
	tclPlatStubsPtr = NULL;
	tclIntStubsPtr = NULL;
	tclIntPlatStubsPtr = NULL;
    }

    return actualVersion;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
