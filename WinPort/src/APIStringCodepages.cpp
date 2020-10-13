#include <set>
#include <string>
#include <locale> 
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>

#include <locale.h>

#if !defined(__APPLE__) && !defined(__FreeBSD__)
# include <alloca.h>
#endif

#include "WinCompat.h"
#include "WinPort.h"
#include "wineguts.h"
#include "PathHelpers.h"
#include "ConvertUTF.h"

#include <iconv.h>

#define IsLocaleMatches(current, wanted_literal) \
	( strncmp((current), wanted_literal, sizeof(wanted_literal) - 1) == 0 && \
	( (current)[sizeof(wanted_literal) - 1] == 0 || (current)[sizeof(wanted_literal) - 1] == '.') )

struct Codepages
{
	int oem;
	int ansi;
};

static Codepages DeduceCodepages()
{
	// deduce oem/ansi cp from system locale
	const char *lc = setlocale(LC_CTYPE, NULL);
	if (!lc) {
		fprintf(stderr, "DeduceCodepages: setlocale returned NULL\n");
		return Codepages{866, 1251};
	}

	if (IsLocaleMatches(lc, "af_ZA")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "ar_SA")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_LB")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_EG")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_DZ")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_BH")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_IQ")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_JO")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_KW")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_LY")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_MA")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_OM")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_QA")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_SY")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_TN")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_AE")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ar_YE")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "ast_ES")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "az_AZ")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "az_AZ")) { return Codepages{857, 1254}; }
	if (IsLocaleMatches(lc, "be_BY")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "bg_BG")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "br_FR")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "ca_ES")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "zh_CN")) { return Codepages{936, 936}; }
	if (IsLocaleMatches(lc, "zh_TW")) { return Codepages{950, 950}; }
	if (IsLocaleMatches(lc, "kw_GB")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "cs_CZ")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "cy_GB")) { return Codepages{850, 28604}; }
	if (IsLocaleMatches(lc, "da_DK")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "de_AT")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "de_LI")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "de_LU")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "de_CH")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "de_DE")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "el_GR")) { return Codepages{737, 1253}; }
	if (IsLocaleMatches(lc, "en_AU")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "en_CA")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "en_GB")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "en_IE")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "en_JM")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "en_BZ")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "en_PH")) { return Codepages{437, 1252}; }
	if (IsLocaleMatches(lc, "en_ZA")) { return Codepages{437, 1252}; }
	if (IsLocaleMatches(lc, "en_TT")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "en_US")) { return Codepages{437, 1252}; }
	if (IsLocaleMatches(lc, "en_ZW")) { return Codepages{437, 1252}; }
	if (IsLocaleMatches(lc, "en_NZ")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_PA")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_BO")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_CR")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_DO")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_SV")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_EC")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_GT")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_HN")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_NI")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_CL")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_MX")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_ES")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_CO")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_ES")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_PE")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_AR")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_PR")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_VE")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_UY")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "es_PY")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "et_EE")) { return Codepages{775, 1257}; }
	if (IsLocaleMatches(lc, "eu_ES")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "fa_IR")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "fi_FI")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "fo_FO")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "fr_FR")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "fr_BE")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "fr_CA")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "fr_LU")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "fr_MC")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "fr_CH")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "ga_IE")) { return Codepages{437, 1252}; }
	if (IsLocaleMatches(lc, "gd_GB")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "gv_IM")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "gl_ES")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "he_IL")) { return Codepages{862, 1255}; }
	if (IsLocaleMatches(lc, "hr_HR")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "hu_HU")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "id_ID")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "is_IS")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "it_IT")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "it_CH")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "iv_IV")) { return Codepages{437, 1252}; }
	if (IsLocaleMatches(lc, "ja_JP")) { return Codepages{932, 932}; }
	if (IsLocaleMatches(lc, "kk_KZ")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "ko_KR")) { return Codepages{949, 949}; }
	if (IsLocaleMatches(lc, "ky_KG")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "lt_LT")) { return Codepages{775, 1251}; }
	if (IsLocaleMatches(lc, "lv_LV")) { return Codepages{775, 1257}; }
	if (IsLocaleMatches(lc, "mk_MK")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "mn_MN")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "ms_BN")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "ms_MY")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "nl_BE")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "nl_NL")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "nl_SR")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "nn_NO")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "nb_NO")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "pl_PL")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "pt_BR")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "pt_PT")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "rm_CH")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "ro_RO")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "ru_RU")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "sk_SK")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "sl_SI")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "sq_AL")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "sr_RS")) { return Codepages{855, 1251}; }
	if (IsLocaleMatches(lc, "sr_RS")) { return Codepages{852, 1250}; }
	if (IsLocaleMatches(lc, "sv_SE")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "sv_FI")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "sw_KE")) { return Codepages{437, 1252}; }
	if (IsLocaleMatches(lc, "th_TH")) { return Codepages{874, 874}; }
	if (IsLocaleMatches(lc, "tr_TR")) { return Codepages{857, 1254}; }
	if (IsLocaleMatches(lc, "tt_RU")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "uk_UA")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "ur_PK")) { return Codepages{720, 1256}; }
	if (IsLocaleMatches(lc, "uz_UZ")) { return Codepages{866, 1251}; }
	if (IsLocaleMatches(lc, "uz_UZ")) { return Codepages{857, 1254}; }
	if (IsLocaleMatches(lc, "vi_VN")) { return Codepages{1258, 1258}; }
	if (IsLocaleMatches(lc, "wa_BE")) { return Codepages{850, 1252}; }
	if (IsLocaleMatches(lc, "zh_HK")) { return Codepages{950, 950}; }
	if (IsLocaleMatches(lc, "zh_SG")) { return Codepages{936, 936}; }
	if (IsLocaleMatches(lc, "zh_MO")) { return Codepages{950, 1252}; }

	fprintf(stderr, "DeduceCodepages: unknown locale '%s'\n", lc);

	return Codepages{866, 1251};
}

static UINT TranslateCodepage(UINT codepage)
{
	static Codepages s_cp = DeduceCodepages();
	switch (codepage) {
		case CP_ACP: return s_cp.ansi;
		case CP_OEMCP: return s_cp.oem;
		default:
			return codepage;
	}
}


extern "C" {
	WINPORT_DECL(IsTextUnicode, BOOL, (CONST VOID* buf, int len, LPINT pf))
	{//borrowed from wine
		static const WCHAR std_control_chars[] = {'\r','\n','\t',' ',0x3000,0};
		static const WCHAR byterev_control_chars[] = {0x0d00,0x0a00,0x0900,0x2000,0};
		const WCHAR *s = (const WCHAR *)buf;
		int i;
		unsigned int flags = ~0U, out_flags = 0;

		if (len < (int)sizeof(WCHAR))
		{
			/* FIXME: MSDN documents IS_TEXT_UNICODE_BUFFER_TOO_SMALL but there is no such thing... */
			if (pf) *pf = 0;
			return FALSE;
		}
		if (pf)
			flags = *pf;
		/*
		* Apply various tests to the text string. According to the
		* docs, each test "passed" sets the corresponding flag in
		* the output flags. But some of the tests are mutually
		* exclusive, so I don't see how you could pass all tests ...
		*/

		/* Check for an odd length ... pass if even. */
		if (len & 1) out_flags |= IS_TEXT_UNICODE_ODD_LENGTH;

		if (((const char *)buf)[len - 1] == 0)
			len--;  /* Windows seems to do something like that to avoid e.g. false IS_TEXT_UNICODE_NULL_BYTES  */

		len /= sizeof(WCHAR);
		/* Windows only checks the first 256 characters */
		if (len > 256) len = 256;

		/* Check for the special byte order unicode marks. */
		if (*s == 0xFEFF) out_flags |= IS_TEXT_UNICODE_SIGNATURE;
		if (*s == 0xFFFE) out_flags |= IS_TEXT_UNICODE_REVERSE_SIGNATURE;

		/* apply some statistical analysis */
		if (flags & IS_TEXT_UNICODE_STATISTICS)
		{
			int stats = 0;
			/* FIXME: checks only for ASCII characters in the unicode stream */
			for (i = 0; i < len; i++)
			{
				if (s[i] <= 255) stats++;
			}
			if (stats > len / 2)
				out_flags |= IS_TEXT_UNICODE_STATISTICS;
		}

		/* Check for unicode NULL chars */
		if (flags & IS_TEXT_UNICODE_NULL_BYTES)
		{
			for (i = 0; i < len; i++)
			{
				if (!(s[i] & 0xff) || !(s[i] >> 8))
				{
					out_flags |= IS_TEXT_UNICODE_NULL_BYTES;
					break;
				}
			}
		}

		if (flags & IS_TEXT_UNICODE_CONTROLS)
		{
			for (i = 0; i < len; i++)
			{
				if (wcschr(std_control_chars, s[i]))
				{
					out_flags |= IS_TEXT_UNICODE_CONTROLS;
					break;
				}
			}
		}

		if (flags & IS_TEXT_UNICODE_REVERSE_CONTROLS)
		{
			for (i = 0; i < len; i++)
			{
				if (wcschr(byterev_control_chars, s[i]))
				{
					out_flags |= IS_TEXT_UNICODE_REVERSE_CONTROLS;
					break;
				}
			}
		}

		if (pf)
		{
			out_flags &= *pf;
			*pf = out_flags;
		}
		/* check for flags that indicate it's definitely not valid Unicode */
		if (out_flags & (IS_TEXT_UNICODE_REVERSE_MASK | IS_TEXT_UNICODE_NOT_UNICODE_MASK)) return FALSE;
		/* now check for invalid ASCII, and assume Unicode if so */
		if (out_flags & IS_TEXT_UNICODE_NOT_ASCII_MASK) return TRUE;
		/* now check for Unicode flags */
		if (out_flags & IS_TEXT_UNICODE_UNICODE_MASK) return TRUE;
		/* no flags set */
		return FALSE;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	
	/***********************************************************************
	*              MultiByteToWideChar   (KERNEL32.@)
	*
	* Convert a multibyte character string into a Unicode string.
	*
	* PARAMS
	*   page   [I] Codepage character set to convert from
	*   flags  [I] Character mapping flags
	*   src    [I] Source string buffer
	*   srclen [I] Length of src (in bytes), or -1 if src is NUL terminated
	*   dst    [O] Destination buffer
	*   dstlen [I] Length of dst (in WCHARs), or 0 to compute the required length
	*
	* RETURNS
	*   Success: If dstlen > 0, the number of characters written to dst.
	*            If dstlen == 0, the number of characters needed to perform the
	*            conversion. In both cases the count includes the terminating NUL.
	*   Failure: 0. Use GetLastError() to determine the cause. Possible errors are
	*            ERROR_INSUFFICIENT_BUFFER, if not enough space is available in dst
	*            and dstlen != 0; ERROR_INVALID_PARAMETER,  if an invalid parameter
	*            is passed, and ERROR_NO_UNICODE_TRANSLATION if no translation is
	*            possible for src.
	*/
	WINPORT_DECL(MultiByteToWideChar, int, ( UINT page, DWORD flags, 
		LPCSTR src, int srclen, LPWSTR dst, int dstlen))
	{

        int trcp = TranslateCodepage(page);
        if (true)
        {
        	//fprintf(stderr, "\n\n\nMultiByteToWideChar for page %d (translated %d)\n\n\n", page, trcp);

            size_t source_len;
            if (srclen == -1) {
                source_len = strlen(src);
            } else {
                source_len = srclen;
            }
            size_t source_len_orig = source_len;

            size_t dest_bytes = source_len * 2 + 2; // dest buf size: maximum possible from something to utf16le
            size_t dest_bytes_orig = dest_bytes;

            char *out_buf = (char*)malloc(dest_bytes);
            char *out_buf_orig = out_buf;

            char *cp_in = (char*)malloc(20);
            if      (trcp == CP_KOI8R)       { sprintf(cp_in, "KOI8-R"); }
            else if (trcp == CP_UTF7)        { sprintf(cp_in, "UTF-7"); }
            else if (trcp == CP_UTF8)        { sprintf(cp_in, "UTF-8"); }
            else if (trcp == CP_UTF16LE)     { sprintf(cp_in, "UTF-16LE"); }
            else if (trcp == CP_UTF16BE)     { sprintf(cp_in, "UTF-16BE"); }
            else if (trcp == CP_UTF32LE)     { sprintf(cp_in, "UTF-32LE"); }
            else if (trcp == CP_UTF32BE)     { sprintf(cp_in, "UTF-32BE"); }
            else                             { sprintf(cp_in, "CP%d", trcp); }
            iconv_t cd = iconv_open("UTF-16LE", cp_in);
            free(cp_in);

            char *src_copy = (char*)src;

            int res = iconv(cd, (char**)&src_copy, &source_len, &out_buf, &dest_bytes);
            iconv_close(cd);
            if (res < 0) {
                free(out_buf_orig);
                //fprintf(stderr, "\n\niconv err: %d\n", errno);
                //fprintf(stderr, "strerror: %s\n", strerror(errno));
                WINPORT(SetLastError)( ERROR_NO_UNICODE_TRANSLATION );
                //WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
                return 0;
            }

            size_t done_bytes = dest_bytes_orig - dest_bytes;

            if (dstlen == 0) {
                // needed size detection mode
                free(out_buf_orig);
                //fprintf(stderr, "calc: %d\n", done_bytes/2 + 1);
                return done_bytes/2 + 1; // in utf16le chars, not bytes! + zero terminator
            }

            // dst is 4-byte wchar_t here, but iconv gives us double byte utf16l,
            // so converting to far2l's "utf16le-inside-32bit-wchar_t"
            char *dst2 = (char*)dst;

            if (done_bytes/2 > dstlen) {
                WINPORT(SetLastError)( ERROR_INSUFFICIENT_BUFFER );
                return 0;
            }

            for (int i=0;i<done_bytes*2;i+=4) {
                memcpy(dst2 + i, out_buf_orig + i/2, 2);
                dst2[i+2] = 0;
                dst2[i+3] = 0;
            }

            // zero terminate
            // but this breaks multiarc
            //bzero(dst2 + done_bytes*2, 4);

            free(out_buf_orig);

            /*
            fprintf(stderr, "source_len_orig: %d\n", source_len_orig);
            fprintf(stderr, "dstlen: %d\n", dstlen);
            fprintf(stderr, "source: %s\n", src);
            fprintf(stderr, "decoded as: %ls\n", dst);
            fprintf(stderr, "done: %d\n", done_bytes/2);
            */

            return done_bytes/2; // in utf16le chars, not bytes!
        }
	}


	/***********************************************************************
	*              WideCharToMultiByte   (KERNEL32.@)
	*
	* Convert a Unicode character string into a multibyte string.
	*
	* PARAMS
	*   page    [I] Code page character set to convert to
	*   flags   [I] Mapping Flags (MB_ constants from "winnls.h").
	*   src     [I] Source string buffer
	*   srclen  [I] Length of src (in WCHARs), or -1 if src is NUL terminated
	*   dst     [O] Destination buffer
	*   dstlen  [I] Length of dst (in bytes), or 0 to compute the required length
	*   defchar [I] Default character to use for conversion if no exact
	*		    conversion can be made
	*   used    [O] Set if default character was used in the conversion
	*
	* RETURNS
	*   Success: If dstlen > 0, the number of characters written to dst.
	*            If dstlen == 0, number of characters needed to perform the
	*            conversion. In both cases the count includes the terminating NUL.
	*   Failure: 0. Use GetLastError() to determine the cause. Possible errors are
	*            ERROR_INSUFFICIENT_BUFFER, if not enough space is available in dst
	*            and dstlen != 0, and ERROR_INVALID_PARAMETER, if an invalid
	*            parameter was given.
	*/
	WINPORT_DECL(WideCharToMultiByte, int, ( UINT page, DWORD flags, LPCWSTR src, 
		int srclen, LPSTR dst, int dstlen, LPCSTR defchar, LPBOOL used))
	{

        //fprintf(stderr, "\n\n\n WC->MB src: %s \n\n\n", src);

        int trcp = TranslateCodepage(page);
        if (true)
        {
        	//fprintf(stderr, "\n\n\nWideCharToMultiByte for page %d (translated %d)\n\n\n", page, trcp);

            size_t source_len;
            if (srclen == -1) {
                source_len = wcslen(src);
            } else {
                source_len = srclen;
            }

            size_t dest_bytes = source_len * 4 + 4; // dest buf size: maximum possible from utf16le to something
            size_t dest_bytes_orig = dest_bytes;

            int zsize;

            char *cp_out = (char*)malloc(20);
            if      (trcp == CP_KOI8R)       { sprintf(cp_out, "KOI8-R");     zsize = 1; }
            else if (trcp == CP_UTF7)        { sprintf(cp_out, "UTF-7");      zsize = 1; }
            else if (trcp == CP_UTF8)        { sprintf(cp_out, "UTF-8");      zsize = 1; }
            else if (trcp == CP_UTF16LE)     { sprintf(cp_out, "UTF-16LE");   zsize = 2; }
            else if (trcp == CP_UTF16BE)     { sprintf(cp_out, "UTF-16BE");   zsize = 2; }
            else if (trcp == CP_UTF32LE)     { sprintf(cp_out, "UTF-32LE");   zsize = 4; }
            else if (trcp == CP_UTF32BE)     { sprintf(cp_out, "UTF-32BE");   zsize = 4; }
            else                             { sprintf(cp_out, "CP%d", trcp); zsize = 1; }
            iconv_t cd = iconv_open(cp_out, "UTF-16LE");
            free(cp_out);

            // src is 4-byte wchar_t here, but iconv requires double byte utf16l,
            // so converting from far2l's "utf16le-inside-32bit-wchar_t"
            char *in_buf = (char*)malloc(source_len * 2 + 2);
            char *in_buf_orig = in_buf;
            char *src2 = (char*)src;
            for (int i=0;i<source_len*2;i+=2) {
                in_buf[i] = src2[i*2];
                in_buf[i+1] = src2[i*2+1];
            }
            bzero(in_buf + source_len*2, 2);

            size_t in_buf_size = source_len * 2;
            size_t in_buf_size_orig = in_buf_size;

            char *out_buf = (char*)malloc(source_len * 4 + 4); // maximum possible
            char *out_buf_orig = out_buf;
            int res = iconv(cd, (char**)&in_buf, &in_buf_size, &out_buf, &dest_bytes);
            free(in_buf_orig);
            iconv_close(cd);
            if (res < 0) {
                //fprintf(stderr, "\n\nwc2mb iconv err (detect size): %d\n", errno);
                //fprintf(stderr, "strerror: %s\n", strerror(errno));
                WINPORT(SetLastError)( ERROR_NO_UNICODE_TRANSLATION );
                //WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
                return 0;
            }

            size_t done_bytes = dest_bytes_orig - dest_bytes;

            if (dstlen == 0) {
                free(out_buf_orig);
                // size detection mode
                //fprintf(stderr, "calc: %d\n", done_bytes);
                return done_bytes + zsize;
            }

            if (done_bytes > dstlen) {
                free(out_buf_orig);
                WINPORT(SetLastError)( ERROR_INSUFFICIENT_BUFFER );
                return 0;
            }

            memcpy(dst, out_buf_orig, done_bytes);
            bzero(dst + done_bytes, zsize);
            free(out_buf_orig);
            
            /*
            fprintf(stderr, "\n\done: %d\n", done_bytes);
            fprintf(stderr, "dstlen: %d\n", dstlen);
            fprintf(stderr, "ibso: %d\n", in_buf_size_orig);
            fprintf(stderr, "dbo: %d\n", dest_bytes_orig);
            fprintf(stderr, "res: %d\n", res);
            fprintf(stderr, "source: %ls\n", src);
            fprintf(stderr, "decoded as: %s\n", (char*)dst);
            */

            return done_bytes;
        }
	}

	WINPORT_DECL(GetOEMCP, UINT, ())
	{
		return TranslateCodepage(CP_OEMCP);//get_codepage_table( 866 )->info.codepage;//866
	}

	WINPORT_DECL(GetACP, UINT, ())
	{
		return TranslateCodepage(CP_ACP);//get_codepage_table( 1251 )->info.codepage;//1251
	}

	WINPORT_DECL(GetCPInfo, BOOL, (UINT codepage, LPCPINFO cpinfo))
	{
		const union cptable *table;

		if (!cpinfo)
		{
			WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
			return FALSE;
		}
		memset(cpinfo, 0, sizeof(*cpinfo));

		codepage = TranslateCodepage(codepage);

		//if (!(table = get_codepage_table( codepage )))
		{
			switch(codepage) {
			case CP_UTF7:
			case CP_UTF8:
				cpinfo->DefaultChar[0] = 0x3f;
				cpinfo->MaxCharSize = (codepage == CP_UTF7) ? 5 : 4;
				return TRUE;

			case CP_UTF16LE: 
				cpinfo->DefaultChar[0] = 0x3f;
				cpinfo->MaxCharSize = 2;
				return TRUE;

			case CP_UTF16BE:
				cpinfo->DefaultChar[1] = 0x3f;
				cpinfo->MaxCharSize = 2;
				return TRUE;
				
			case CP_UTF32LE: 
				cpinfo->DefaultChar[0] = 0x3f;
				cpinfo->MaxCharSize = 4;
				return TRUE;

			case CP_UTF32BE:
				cpinfo->DefaultChar[3] = 0x3f;
				cpinfo->MaxCharSize = 4;
				return TRUE;
			}
			fprintf(stderr, "GetCPInfo: bad codepage %u\n", codepage);
			WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
			return FALSE;
		}
        /*
		if (table->info.def_char & 0xff00)
		{
			cpinfo->DefaultChar[0] = (table->info.def_char & 0xff00) >> 8;
			cpinfo->DefaultChar[1] = table->info.def_char & 0x00ff;
		}
		else
		{
			cpinfo->DefaultChar[0] = table->info.def_char & 0xff;
			cpinfo->DefaultChar[1] = 0;
		}
		if ((cpinfo->MaxCharSize = table->info.char_size) == 2)
			memcpy( cpinfo->LeadByte, table->dbcs.lead_bytes, sizeof(cpinfo->LeadByte) );
		else
			cpinfo->LeadByte[0] = cpinfo->LeadByte[1] = 0;

		return TRUE;
        */
	}
	
	WINPORT_DECL(GetCPInfoEx, BOOL, (UINT codepage, DWORD dwFlags, LPCPINFOEX cpinfo))
	{
		if (!WINPORT(GetCPInfo)( codepage, (LPCPINFO)cpinfo ))
			return FALSE;

		switch(codepage) {
			case CP_UTF7:
			{
				static const WCHAR utf7[] = {'U','n','i','c','o','d','e',' ','(','U','T','F','-','7',')',0};

				cpinfo->CodePage = CP_UTF7;
				cpinfo->UnicodeDefaultChar = 0x3f;
				strcpyW(cpinfo->CodePageName, utf7);
				break;
			}

			case CP_UTF8:
			{
				static const WCHAR utf8[] = {'U','n','i','c','o','d','e',' ','(','U','T','F','-','8',')',0};

				cpinfo->CodePage = CP_UTF8;
				cpinfo->UnicodeDefaultChar = 0x3f;
				strcpyW(cpinfo->CodePageName, utf8);
				break;
			}

			case CP_UTF16LE:
			{
				cpinfo->CodePage = CP_UTF16LE;
				cpinfo->UnicodeDefaultChar = 0x3f;
				wcscpy(cpinfo->CodePageName, L"Unicode (UTF-16 LE)");
				break;
			}

			case CP_UTF16BE:
			{
				cpinfo->CodePage = CP_UTF16BE;
				cpinfo->UnicodeDefaultChar = 0x003f;
				wcscpy(cpinfo->CodePageName, L"Unicode (UTF-16 BE)");
				break;
			}

			case CP_UTF32LE:
			{
				cpinfo->CodePage = CP_UTF32LE;
				cpinfo->UnicodeDefaultChar = 0x3f;
				wcscpy(cpinfo->CodePageName, L"Unicode (UTF-32 LE)");
				break;
			}

			case CP_UTF32BE:
			{
				cpinfo->CodePage = CP_UTF32BE;
				cpinfo->UnicodeDefaultChar = 0x0000003f;
				wcscpy(cpinfo->CodePageName, L"Unicode (UTF-32 BE)");
				break;
			}

			default:
			{
                return FALSE;
				/*
                const union cptable *table = get_codepage_table( codepage );
				if (!table)
					return FALSE;

				cpinfo->CodePage = table->info.codepage;
				cpinfo->UnicodeDefaultChar = table->info.def_unicode_char;
				WINPORT(MultiByteToWideChar)( CP_ACP, 0, table->info.name, -1, cpinfo->CodePageName,
                                 sizeof(cpinfo->CodePageName)/sizeof(WCHAR));
                */
				break;
			}
		}
		return TRUE;
	}

	WINPORT_DECL(EnumSystemCodePages, BOOL, (CODEPAGE_ENUMPROCW lpfnCodePageEnum, DWORD flags))
	{
        return TRUE; //FIXME

	    const union cptable *table;
	    WCHAR buffer[10], *p;
	    int page, index = 0;
	    for (;;)
	    {
        	//if (!(table = wine_cp_enum_table( index++ ))) break;
	        p = buffer + sizeof(buffer)/sizeof(WCHAR);
	        *--p = 0;
	        page = table->info.codepage;
	        do {
        	    *--p = '0' + (page % 10);
	            page /= 10;
	        } while( page );
        	if (!lpfnCodePageEnum( p )) break;
	    }
	    return TRUE;
	}
}

