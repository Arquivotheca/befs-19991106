/*
 *  linux/fs/befs/util.c
 *
 * Copyright (C) 1999  Makoto Kato (m_kato@ga2.so-net.ne.jp)
 */

#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/nls.h>

#define BEFS_UNKNOWN_CHAR '?'

/*
 * UTF-8 to NLS charset  convert routine
 */

char * befs_utf2nls (char * src, int srclen, char ** dist, int * len, struct super_block * sb)
{
	char *           out;
	unsigned char *  page;
	struct nls_table * nls = sb->u.befs_sb.nls;
	__u16            w;
	int              n;

	BEFS_OUTPUT (("---> utf2nls()\n"));

	*dist = out = (char *) __getname();
	if (!out) {
		BEFS_OUTPUT ((" cannot allocate memory\n"));
		return NULL;
	}

	if (!nls) {
		while (*src && srclen-- > 0)
			*out++ = *src++;
	} else {
		while (*src && srclen > 0) {
			/*
			 *  convert from UTF-8 to Unicode
			 */

			n = utf8_mbtowc(&w, src, srclen);
			if (n <= 0) {
				/*
				 * cannot convert from UTF-8 to unicode
				 */

				*out++ = BEFS_UNKNOWN_CHAR;
				src++;
				srclen--;
				continue;
			}

			src += n;
			srclen -= n;

			/*
			 * convert from unicode to nls
			 */

			page = nls->page_uni2charset[w >> 8];
			if (!page)
				*out++ = BEFS_UNKNOWN_CHAR;
			else
				*out++ = page[w & 0xff];
		}
	}
	*out = '\0';
	*len = out - (*dist);

	BEFS_OUTPUT (("<--- utf2nls()\n"));

	return *dist;
}


char * befs_nls2utf (char * src, int srclen, char ** dist, int * len, struct super_block * sb)
{
	char *  out;
	__u16   w = 0;
	struct nls_table * nls = sb->u.befs_sb.nls;
	int     n;

	BEFS_OUTPUT (("---> nls2utf()\n"));

	*dist = out = (char *) __getname();
	if (!out) {
		BEFS_OUTPUT ((" cannot allocate memory\n"));
		return NULL;
	}

	if (!nls) {
		while (*src && srclen-- > 0)
			*out++ = *src++;
	} else {
		while (*src && srclen > 0) {
			/*
			 * convert from nls to unicode
			 */

			w = (((__u16) nls->charset2uni[(int) *src].uni2) << 8)
				+ nls->charset2uni[(int) *src].uni1;
			src++;
			srclen--;

			/*
			 * convert from unicode to UTF-8
			 */

			n = utf8_wctomb(out, w, 6);
			if (n <= 0) {
				*out++ = BEFS_UNKNOWN_CHAR;
				continue;
			}

			out += n;
		}
	}

	*out = '\0';
	*len = out - (*dist);

	BEFS_OUTPUT (("<--- nls2utf()\n"));

	return *dist;
}
