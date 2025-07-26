#include "headers.hpp"

#include "mix.hpp"
#include <mutex>
#include <vector>
#include <list>
#include <fcntl.h>
#include "config.hpp"
#include <WideMB.h>

#include "vtlog.h"

#include "vtshell.h"
#include "ctrlobj.hpp"
#include "cmdline.hpp"


#define FOREGROUND_RGB (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE)
#define BACKGROUND_RGB (BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE)

namespace VTLog
{
	struct DumpState
	{
		DumpState() : nonempty(false) {}
		
		bool nonempty;
	};

	static inline unsigned char TranslateForegroundColor(WORD attributes)
	{
		unsigned char out = 0;
		if (attributes&FOREGROUND_RED) out|= 1;
		if (attributes&FOREGROUND_GREEN) out|= 2;
		if (attributes&FOREGROUND_BLUE) out|= 4;
		return out;
	}

	static inline unsigned char TranslateBackgroundColor(WORD attributes)
	{
		unsigned char out = 0;
		if (attributes&BACKGROUND_RED) out|= 1;
		if (attributes&BACKGROUND_GREEN) out|= 2;
		if (attributes&BACKGROUND_BLUE) out|= 4;
		return out;
	}


	static unsigned int ActualLineWidth(unsigned int Width, const CHAR_INFO *Chars)
	{
		for (;;) {
			if (!Width)
				return 0;

			--Width;
			const auto &CI = Chars[Width];
			if ((CI.Char.UnicodeChar && CI.Char.UnicodeChar != L' ') || (CI.Attributes & BACKGROUND_RGB) != 0) {
				return Width + 1;
			}
		}
	}

	static void EncodeLine(std::string &out, unsigned int Width, const CHAR_INFO *Chars, bool colored)
	{
		DWORD64 attr_prev = (DWORD64)-1;
		for (unsigned int i = 0; i < Width; ++i) if (Chars[i].Char.UnicodeChar) {
			const DWORD64 attr_now = Chars[i].Attributes;
			if ( colored && attr_now != attr_prev) {
				const bool tc_back_now = (attr_now & BACKGROUND_TRUECOLOR) != 0;
				const bool tc_back_prev = (attr_prev & BACKGROUND_TRUECOLOR) != 0;
				const bool tc_fore_now = (attr_now & FOREGROUND_TRUECOLOR) != 0;
				const bool tc_fore_prev = (attr_prev & FOREGROUND_TRUECOLOR) != 0;

				const size_t out_len_before_attr = out.size();
				out+= "\033[";
				if ( attr_prev == (DWORD64)-1
				|| (attr_prev&FOREGROUND_INTENSITY) != (attr_now&FOREGROUND_INTENSITY)) {
					out+= (attr_now&FOREGROUND_INTENSITY) ? "1;" : "22;";
				}
				if ( attr_prev == (DWORD64)-1 || (tc_fore_prev && !tc_fore_now)
				|| (attr_prev&(FOREGROUND_INTENSITY|FOREGROUND_RGB)) != (attr_now&(FOREGROUND_INTENSITY|FOREGROUND_RGB))) {
					out+= (attr_now&FOREGROUND_INTENSITY) ? '9' : '3';
					out+= '0' + TranslateForegroundColor(attr_now);
					out+= ';';
				}
				if ( attr_prev == (DWORD64)-1 || (tc_back_prev && !tc_back_now)
				|| (attr_prev&(BACKGROUND_INTENSITY|BACKGROUND_RGB)) != (attr_now&(BACKGROUND_INTENSITY|BACKGROUND_RGB))) {
					out+= (attr_now&BACKGROUND_INTENSITY) ? "10" : "4";
					out+= '0' + TranslateBackgroundColor(attr_now);
					out+= ';';
				}

				if (tc_fore_now && (!tc_fore_prev || GET_RGB_FORE(attr_prev) != GET_RGB_FORE(attr_now))) {
					const DWORD rgb = GET_RGB_FORE(attr_now);
					out+= StrPrintf("38;2;%u;%u;%u;", rgb & 0xff, (rgb >> 8) & 0xff, (rgb >> 16) & 0xff);
				}

				if (tc_back_now && (!tc_back_prev || GET_RGB_BACK(attr_prev) != GET_RGB_BACK(attr_now))) {
					const DWORD rgb = GET_RGB_BACK(attr_now);
					out+= StrPrintf("48;2;%u;%u;%u;", rgb & 0xff, (rgb >> 8) & 0xff, (rgb >> 16) & 0xff);
				}

				if (out.back() == ';') {
					out.back() = 'm';
					attr_prev = attr_now;

				} else { // no visible attributes changed --> dismiss sequence start
					out.resize(out_len_before_attr);
				}
			}

			if (CI_USING_COMPOSITE_CHAR(Chars[i])) {
				const wchar_t *pwc = WINPORT(CompositeCharLookup)(Chars[i].Char.UnicodeChar);
				Wide2MB_UnescapedAppend(pwc, wcslen(pwc), out);

			} else if (Chars[i].Char.UnicodeChar > 0x80) {
				Wide2MB_UnescapedAppend(Chars[i].Char.UnicodeChar, out);

			} else {
				out+= (char)(unsigned char)Chars[i].Char.UnicodeChar;
			}
		}
		if (colored && attr_prev != 0xffff) {
			out+= "\033[m";
		}
	}

	
	static class Lines
	{
		struct LogLine
		{
			HANDLE con_hnd;
			std::string text;
			bool wrapped;
		};

		std::mutex _mutex;
		std::list<LogLine> _memories;
		
	public:
		void Add(HANDLE con_hnd, unsigned int actual_width, const CHAR_INFO *Chars, bool is_wrapped)
		{
			if (actual_width == 0)
				return;

			std::lock_guard<std::mutex> lock(_mutex);

			if (!_memories.empty() && _memories.back().con_hnd == con_hnd && _memories.back().wrapped)
			{
				auto &last = _memories.back();
				EncodeLine(last.text, actual_width, Chars, true);
				last.wrapped = is_wrapped;
				return;
			}

			const size_t limit = (size_t)std::max(Opt.CmdLine.VTLogLimit, 2);
			if (_memories.size() >= limit) {
				while (_memories.size() > limit) {
					_memories.pop_front();
				}
				_memories.splice(_memories.end(), _memories, _memories.begin());
			} else {
				_memories.emplace_back();
			}

			auto &last = _memories.back();
			last.con_hnd = con_hnd;
			last.text.clear();
			EncodeLine(last.text, actual_width, Chars, true);
			last.wrapped = is_wrapped;
		}
		
		void DumpTo(HANDLE con_hnd, std::string &s, DumpState &ds, bool colored, bool &was_wrapped)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			was_wrapped = false;
			for (const auto& m : _memories)
			{
				if (m.con_hnd == con_hnd)
				{
					if (!m.text.empty() || ds.nonempty)
					{
						ds.nonempty = true;

						if (!s.empty() && !was_wrapped)
						{
							s += NATIVE_EOL;
						}

						std::string temp_text = m.text;
						if (!colored)
						{
							for (;;) {
								size_t i = temp_text.find('\033');
								if (i == std::string::npos) break;
								size_t j = temp_text.find('m', i + 1);
								if (j == std::string::npos) break;
								temp_text.erase(i, j + 1 - i);
							}
						}
						s += temp_text;
						was_wrapped = m.wrapped;
					}
				}
			}
		}
		
		void Reset(HANDLE con_hnd)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			for (auto it = _memories.begin(); it != _memories.end();) {
				if (it->con_hnd == con_hnd) {
					it = _memories.erase(it);
				} else {
					++it;
				}
			}
		}

		void ConsoleJoined(HANDLE con_hnd)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			size_t remain = _memories.size();
			for (auto it = _memories.begin(); remain; --remain) {
				if (it->con_hnd == con_hnd) {
					auto next = it;
					++next;
					it->con_hnd = NULL;
					_memories.splice(_memories.end(), _memories, it);
					it = next;
				} else {
					++it;
				}
			}
		}

	} g_lines;

	static unsigned int g_pause_cnt = 0;

	void OnConsoleScroll(PVOID pContext, HANDLE hConsole, unsigned int Width, CHAR_INFO *Chars)
	{
		if (g_pause_cnt == 0) {
			const unsigned int actual_width = ActualLineWidth(Width, Chars);
			const bool is_wrapped = (actual_width >= Width);
			g_lines.Add(hConsole, actual_width, Chars, is_wrapped);
		}
	}

	void Pause()
	{
		__sync_add_and_fetch(&g_pause_cnt, 1);
	}

	void Resume()
	{
		if (__sync_sub_and_fetch(&g_pause_cnt, 1) < 0) {
			ABORT();
		}
	}
	
	void Start()
	{
		WINPORT(SetConsoleScrollCallback) (NULL, OnConsoleScroll, NULL);
	}

	void Stop()
	{
		WINPORT(SetConsoleScrollCallback) (NULL, NULL, NULL);
	}

	void ConsoleJoined(HANDLE con_hnd)
	{
		g_lines.ConsoleJoined(con_hnd);
	}
	
	void Reset(HANDLE con_hnd)
	{
		g_lines.Reset(con_hnd);
	}
	
	static void AppendBufferLines(const CHAR_INFO *buffer, int total_height, int line_width, std::string &s, DumpState &ds, bool &was_wrapped, bool colored)
	{
		const CHAR_INFO *current_line = buffer;
		for (int y = 0; y < total_height; ++y, current_line += line_width)
		{
			unsigned int actual_width = ActualLineWidth((unsigned int)line_width, current_line);

			if (actual_width == 0)
			{
				if (!was_wrapped)
				{
					if (ds.nonempty || !s.empty())
					{
						s += NATIVE_EOL;
					}
				}
				was_wrapped = false;
				continue;
			}

			ds.nonempty = true;

			if (!was_wrapped)
			{
				if (!s.empty())
				{
					s += NATIVE_EOL;
				}
			}

			EncodeLine(s, actual_width, current_line, colored);
			was_wrapped = (actual_width >= (unsigned int)line_width);
		}
	}

	static void AppendActiveScreenLines(HANDLE con_hnd, std::string &s, DumpState &ds, bool &was_wrapped, bool colored)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi = { };
		if (WINPORT(GetConsoleScreenBufferInfo)(con_hnd, &csbi) && csbi.dwSize.X > 0 && csbi.dwSize.Y > 0) {
			std::vector<CHAR_INFO> screen_buffer(csbi.dwSize.X * csbi.dwSize.Y);
			COORD buf_size = { csbi.dwSize.X, csbi.dwSize.Y };
			COORD buf_coord = { 0, 0 };
			SMALL_RECT read_region = { 0, 0, (SHORT)(csbi.dwSize.X - 1), (SHORT)(csbi.dwSize.Y - 1) };

			if (WINPORT(ReadConsoleOutput)(con_hnd, &screen_buffer[0], buf_size, buf_coord, &read_region))
			{
				AppendBufferLines(&screen_buffer[0], csbi.dwSize.Y, csbi.dwSize.X, s, ds, was_wrapped, colored);
			}
		}
	}

	static void AppendSavedScreenLines(std::string &s, DumpState &ds, bool &was_wrapped, bool colored)
	{
		if (CtrlObject->CmdLine) {
			int w = 0, h = 0;
			const CHAR_INFO *ci = CtrlObject->CmdLine->GetBackgroundScreen(w, h);
			if (ci && w > 0 && h > 0) {
				AppendBufferLines(ci, h, w, s, ds, was_wrapped, colored);
			}
		}
	}

	std::string GetAsFile(HANDLE con_hnd, bool colored, bool append_screen_lines, const char *wanted_path)
	{
		std::string path;
		if (wanted_path && *wanted_path) {
			path = wanted_path;
		} else {
			SYSTEMTIME st;
			WINPORT(GetLocalTime)(&st);
			path = InMyTempFmt("farvt_%u-%u-%u_%u-%u-%u.%s",
				 st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
				 colored ? "ans" : "log");
		}
				
		int fd = open(path.c_str(), O_CREAT | O_TRUNC | O_RDWR | O_CLOEXEC, 0600);
		if (fd==-1) {
			fprintf(stderr, "VTLog: errno %u creating '%s'\n", errno, path.c_str() );
			return std::string();
		}
			
		std::string total_log_content;
		DumpState ds;
		bool was_wrapped = false;

		g_lines.DumpTo(con_hnd, total_log_content, ds, colored, was_wrapped);

		if (append_screen_lines) {
			if (!con_hnd && !VTShell_Busy()) {
				AppendSavedScreenLines(total_log_content, ds, was_wrapped, colored);
			} else {
				AppendActiveScreenLines(con_hnd, total_log_content, ds, was_wrapped, colored);
			}
		}

		if (!total_log_content.empty()) {
			// Add a final newline for consistency, if the log doesn't already end with one.
			if (!was_wrapped) {
				total_log_content += NATIVE_EOL;
			}
			if (write(fd, total_log_content.c_str(), total_log_content.size()) != (int)total_log_content.size())
				perror("VTLog: write");
		}

		close(fd);
		return path;
	}
}
