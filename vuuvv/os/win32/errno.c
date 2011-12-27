#include "vuuvv.h"

char *
v_strerror(v_err_t err, char *errstr, size_t size)
{
	int             len;
	static long     lang = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

	if (size == 0) {
		return errstr;
	}

	len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, err, lang, errstr, size, NULL);
	if (len == 0 && lang && GetLastError() == ERROR_RESOURCE_LANG_NOT_FOUND) {

		/*
		 * Try to use English messages first and fallback to a language,
		 * based on locale: non-English Windows have no English messages
		 * at all.  This way allows to use English messages at least on
		 * Windows with MUI.
		 */

		lang = 0;

		len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, err, lang, (char *) errstr, size, NULL);
	}

	if (len == 0) {
		_snprintf_s(errstr, size, size,
				"Unknown Error: %d", err);
		return errstr;
	}

	/* remove trailing space */
	while (len > 0 && (errstr[len-1] <= ' '))
		errstr[--len] = L'\0';

	return errstr;
}

int
v_strerror_init(void)
{
	return V_OK;
}
