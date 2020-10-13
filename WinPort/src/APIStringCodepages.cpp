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

        if ((page == CP_MACCP) ||
            (page == CP_THREAD_ACP) ||
            (page == CP_SYMBOL))
        {
            WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
            return 0;
        }

        int trcp = TranslateCodepage(page);

    	//fprintf(stderr, "MultiByteToWideChar, %d --> wchar_t\n", trcp);

        // calculating source length
        size_t source_len;
        if (srclen == -1) {
            source_len = strlen(src) + 1;
        } else {
            source_len = srclen;
        }

        LPCSTR src_orig = src;
        size_t source_len_orig = source_len;

        // calculating destination buffer size, saving for futher use
        size_t dest_bytes = source_len * 4; // maximum possible for "from something to utf32le"
        size_t dest_bytes_orig = dest_bytes;

        // creating output buffer, saving original pointer
        char *out_buf = (char*)malloc(dest_bytes);
        char *out_buf_orig = out_buf;

        // iconv init
        char *cp_in = (char*)malloc(20);
        if      (trcp == CP_KOI8R)       { sprintf(cp_in, "KOI8-R"); }
        else if (trcp == CP_UTF7)        { sprintf(cp_in, "UTF-7"); }
        else if (trcp == CP_UTF8)        { sprintf(cp_in, "UTF-8"); }
        else if (trcp == CP_UTF16LE)     { sprintf(cp_in, "UTF-16LE"); }
        else if (trcp == CP_UTF16BE)     { sprintf(cp_in, "UTF-16BE"); }
        else if (trcp == CP_UTF32LE)     { sprintf(cp_in, "UTF-32LE"); }
        else if (trcp == CP_UTF32BE)     { sprintf(cp_in, "UTF-32BE"); }
        else                             { sprintf(cp_in, "CP%d", trcp); }
        iconv_t cd = iconv_open("UTF-32LE//IGNORE", cp_in);
        free(cp_in);

        if (cd < 0) {
            free(out_buf_orig);
            WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
            return 0;
        }

        // performing conversion
        int res = iconv(cd, (char**)&src, &source_len, &out_buf, &dest_bytes);
        if (res < 0) {
            //fprintf(stderr, "m2w iconv err %d: %s\n", errno, strerror(errno));
            if ((errno == EILSEQ) || (errno == EINVAL)) {
                if (flags & MB_ERR_INVALID_CHARS) {
                    iconv_close(cd);
                    free(out_buf_orig);
                    WINPORT(SetLastError)( ERROR_NO_UNICODE_TRANSLATION );
                    return 0;
                } else {
                    // performing another, fault tolerant conversion attempt, byte by byte
                    //fprintf(stderr, "fault-falerant m2w \n\n\n");
                    src = src_orig;
                    source_len = source_len_orig;
                    out_buf = out_buf_orig;
                    dest_bytes = dest_bytes_orig;
                    // discarding prev conversion attempt
                    bzero(out_buf, dest_bytes);
                    // reset iconv state
                    iconv(cd, NULL, NULL, NULL, NULL);
                    size_t count = 1; // bytes to process per iteration
                    do {
                        // backup vars for retry attempts
                        size_t count_bak = count;
                        LPCSTR src_bak = src;
                        size_t dest_bak = dest_bytes;
                        char *out_buf_bak = out_buf;
                        // reset errno
                        errno = 0;
                        // try iconv
                        res = iconv(cd, (char**)&src, &count, &out_buf, &dest_bytes);

                        if (res < 0) {

                            // ops, faulty or incomplete symbol[s] found

                            // how many symbols were read from input, but not written to output?
                            int wrong = (count_bak - count) - (dest_bak - dest_bytes) / 4;

                            // write "?" placeholders instead
                            for (int i = 0; i < wrong; i++) {
                                *out_buf = '?';
                                out_buf += 4;
                                dest_bytes -= 4;
                            }

                            if (errno == EINVAL) {

                                if (src == src_bak) {
                                    // incomplete char, nothing done?
                                    // retrying with greater buffer
                                    count++;
                                }        

                            } else if (errno == EILSEQ) {

                                if ((count_bak > 1) || (src == src_bak)) {

                                    // wrong char in sequence longer then 1 byte?
                                    // retry from the next byte
                                    src = src_bak + 1;
                                    out_buf = out_buf_bak;
                                    dest_bytes = dest_bak;

                                    // and don't forget to write "?" placeholder
                                    *out_buf = '?';
                                    // it' retry, so let's clear stuff three bytes
                                    bzero(out_buf+1, 3);
                                    out_buf += 4;
                                    dest_bytes -= 4;
                                }

                                count = 1;

                            } else {
                                //fprintf(stderr, "ops, buffer is full, why?\n");
                                break;
                            }
                        } else {
                            // reset buffer size do default 1 byte
                            count = 1;
                        }
                    } while (src < (src_orig + source_len));
                }
            }
        }

        iconv_close(cd);

        // calculating actually written bytes
        size_t done_bytes = dest_bytes_orig - dest_bytes;

        // it's size detection mode?
        if (dstlen == 0) {
            free(out_buf_orig);
            return done_bytes/4; // counted in utf32le chars, not bytes!
        }

        // dst too small?
        if (done_bytes/4 > dstlen) {
            free(out_buf_orig);
            WINPORT(SetLastError)( ERROR_INSUFFICIENT_BUFFER );
            return 0;
        }

        memcpy(dst, out_buf_orig, done_bytes);

        free(out_buf_orig); // out_buf can not be used, it was modified by iconv

        return done_bytes/4; // counted in utf32le chars, not bytes!
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

        if ((page == CP_MACCP) ||
            (page == CP_THREAD_ACP) ||
            (page == CP_SYMBOL))
        {
            WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
            return 0;
        }

        int trcp = TranslateCodepage(page);

    	//fprintf(stderr, "WideCharToMultiByte, wchar_t --> %d\n", trcp);

        // calculating source length
        size_t source_len;
        if (srclen == -1) {
            source_len = wcslen(src) + 1;
        } else {
            source_len = srclen;
        }

        LPCWSTR src_orig = src;

        // source size in bytes
        size_t source_len_bytes = source_len * 4;
        size_t source_len_bytes_orig = source_len_bytes;

        // calculating destination buffer size, saving for futher use
        size_t dest_bytes = source_len_bytes; // "from 32 bit wchar_t[] to something" can't grow in size
        size_t dest_bytes_orig = dest_bytes;

        // allocating conversion output buffer, saving original pointer
        char *out_buf = (char*)malloc(dest_bytes);
        char *out_buf_orig = out_buf;

        // iconv init
        char *cp_out = (char*)malloc(20);
        if      (trcp == CP_KOI8R)       { sprintf(cp_out, "KOI8-R//IGNORE"); }
        else if (trcp == CP_UTF7)        { sprintf(cp_out, "UTF-7//IGNORE"); }
        else if (trcp == CP_UTF8)        { sprintf(cp_out, "UTF-8//IGNORE"); }
        else if (trcp == CP_UTF16LE)     { sprintf(cp_out, "UTF-16LE//IGNORE"); }
        else if (trcp == CP_UTF16BE)     { sprintf(cp_out, "UTF-16BE//IGNORE"); }
        else if (trcp == CP_UTF32LE)     { sprintf(cp_out, "UTF-32LE//IGNORE"); }
        else if (trcp == CP_UTF32BE)     { sprintf(cp_out, "UTF-32BE//IGNORE"); }
        else                             { sprintf(cp_out, "CP%d//IGNORE", trcp); }
        iconv_t cd = iconv_open(cp_out, "UTF-32LE");
        free(cp_out);

        if (cd < 0) {
            free(out_buf_orig);
            WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
            return 0;
        }

        // performing conversion
        int res = iconv(cd, (char**)&src, &source_len_bytes, &out_buf, &dest_bytes);
        if (res < 0) {
            //fprintf(stderr, "w2m iconv err %d: %s\n", errno, strerror(errno));
            if ((errno == EILSEQ) || (errno == EINVAL)) {
                if (flags & MB_ERR_INVALID_CHARS) {
                    iconv_close(cd);
                    free(out_buf_orig);
                    WINPORT(SetLastError)( ERROR_NO_UNICODE_TRANSLATION );
                    return 0;
                } else {
                    // performing another, fault tolerant conversion attempt, char by char
                    /*
                    fprintf(stderr, "fault-falerant w2m \n\n\n");
                    src = src_orig;
                    source_len_bytes = source_len_bytes_orig;
                    out_buf = out_buf_orig;
                    dest_bytes = dest_bytes_orig;
                    // discarding prev conversion attempt
                    bzero(out_buf, dest_bytes);
                    // reset iconv state
                    iconv(cd, NULL, NULL, NULL, NULL);
                    size_t count = 4;
                    do {
                        res = iconv(cd, (char**)&src, &count, &out_buf, &dest_bytes);
                        if (res < 0) {                     // faulty char?
                            if (count == 1) { src += 4; }  // source pointer don't moved? move it
                            *out_buf = '?';                // default char
                            out_buf += 1;                  // move dest pointer
                            dest_bytes -= 1;               // update processed chars count
                        }
                        count = 4; // reset "bytes-to-process" value
                    } while (src < (src_orig + source_len_bytes));
                    */
                }
            }
        }

        iconv_close(cd);

        // calculating actually written bytes
        size_t done_bytes = dest_bytes_orig - dest_bytes;

        // it's size detection mode?
        if (dstlen == 0) {
            free(out_buf_orig);
            return done_bytes;
        }

        // dst too small?
        if (done_bytes > dstlen) {
            free(out_buf_orig);
            WINPORT(SetLastError)( ERROR_INSUFFICIENT_BUFFER );
            return 0;
        }

        // saving conversion result to destination buffer
        memcpy(dst, out_buf_orig, done_bytes);

        free(out_buf_orig); // out_buf can not be used, it was modified by iconv

        return done_bytes;
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

			cpinfo->DefaultChar[3] = 0x3f;
			cpinfo->MaxCharSize = 1;
    		return TRUE;

		    /*	
            fprintf(stderr, "GetCPInfo: bad codepage %u\n", codepage);
			WINPORT(SetLastError)( ERROR_INVALID_PARAMETER );
			return FALSE;
            */
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
        */

		return TRUE;
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
                return TRUE;
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

