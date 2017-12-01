/*
 * SZARP: SCADA software (C) 2017
 *
 * Patryk Kulpanowski <pkulpanowski@newterm.pl>
 */
#ifndef __MINGW32__
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <wx/string.h>

/** Function to validate given config using i2smo
 * @param path path to config */
int ValidateConfig(wxString path);
#endif
