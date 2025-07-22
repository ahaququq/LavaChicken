#pragma once
#include <cmath>
#include <codecvt>
#include <iostream>
#include <string>

namespace wnd {
	inline std::string operator*(const std::string &s, const unsigned long n) {
		std::string out;
		for (unsigned long i = 0; i < n; i++) {
			out += s;
		}
		return out;
	}

	inline std::string set_length(
		std::string in,
		const unsigned long n,
		const std::string &padding = " ",
		const bool end_with_space = false
	) {
		if (end_with_space) {
			in += " ";
		}
		std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
		const auto utf = converter.from_bytes(in);
		if (utf.length() <= n) return in + padding * (n - utf.length());
		return std::string{in.begin(), in.begin() + n};
	}



	inline unsigned long width = 0;
	inline unsigned long frame_level = 0;

	inline std::vector<std::vector<std::string>> columns;
	inline unsigned long current_column;
	inline unsigned long current_row;

	enum WindowButtons {
		none = 0,
		minimise_button = 1,
		maximise_button = 2,
		close_button = 4,
		all_buttons = minimise_button | maximise_button | close_button,
	};

	inline unsigned long begin(const std::string &title, const WindowButtons shown_buttons = none,
					  unsigned long new_width = 0) {
		if (shown_buttons & minimise_button && new_width >= 4) new_width -= 4;
		if (shown_buttons & maximise_button && new_width >= 4) new_width -= 4;
		if (shown_buttons & close_button && new_width >= 4) new_width -= 4;

		new_width = std::max(title.length() + 2, new_width);
		std::cout << "┏" << std::string{"━"} * new_width;

		if (shown_buttons & minimise_button) std::cout << "┯━━━";
		if (shown_buttons & maximise_button) std::cout << "┯━━━";
		if (shown_buttons & close_button) std::cout << "┯━━━";

		std::cout << "┓\n";
		std::cout << "┃ " << set_length(title, new_width - 2) << " ";

		if (shown_buttons & minimise_button) std::cout << "│ - ";
		if (shown_buttons & maximise_button) std::cout << "│ □ ";
		if (shown_buttons & close_button) std::cout << "│ X ";

		std::cout << "┃\n";
		std::cout << "┣" << std::string{"━"} * new_width;

		if (shown_buttons & minimise_button) std::cout << "┷━━━";
		if (shown_buttons & maximise_button) std::cout << "┷━━━";
		if (shown_buttons & close_button) std::cout << "┷━━━";

		std::cout << "┫\n";

		if (shown_buttons & minimise_button) new_width += 4;
		if (shown_buttons & maximise_button) new_width += 4;
		if (shown_buttons & close_button) new_width += 4;

		width = new_width;
		frame_level = 0;

		return new_width;
	}

	inline void begin_section(const std::string &title) {
		std::cout << "┠─ " << set_length(title, width - 3, "─", true) << "─┨\n";
		frame_level = 0;
	}

	inline void print(const std::string &string = "") {
		if (current_column) {
			current_row++;
			while (columns.size() <= current_row) columns.emplace_back();
			for (std::vector<std::string> &row : columns)
				while (row.size() <= current_column - 1) row.emplace_back();
			columns[current_row][current_column - 1] = string;

			return;
		}

		std::cout << "┃";
		std::cout << std::string{"│"} * frame_level;
		std::cout << " " << set_length(string, width - frame_level * 2 - 2) << " ";
		std::cout << std::string{"│"} * frame_level;
		std::cout << "┃\n";
	}

	inline void begin_frame(const std::string &contents = "") {
		std::cout << "┃";
		std::cout << std::string{"│"} * frame_level;
		if (contents.empty()) {
			std::cout << "┌" << std::string{"─"} * (width - 2 - frame_level * 2) << "┐";
		} else {
			std::cout << "┌─ ";
			std::cout << set_length(contents, width - 5 - frame_level * 2, "─", true);
			std::cout << "─┐";
		}
		std::cout << std::string{"│"} * frame_level;
		std::cout << "┃\n";
		frame_level++;
	}

	inline void end_frame() {
		if (frame_level == 0) return;
		frame_level--;

		std::cout << "┃";
		std::cout << std::string{"│"} * frame_level;
		std::cout << "└" << std::string{"─"} * (width - 2 - frame_level * 2) << "┘";
		std::cout << std::string{"│"} * frame_level;
		std::cout << "┃\n";
	}

	inline void end() {
		std::cout << "┗" << std::string{"━"} * width << "┛\n";
		frame_level = 0;
	}

	inline void flush_columns(const bool framed = false, const bool fit_to_width = true) {
		current_column = 0;
		current_row = 0;

		std::vector<unsigned long> column_max_width;

		for (const std::vector<std::string> &row : columns) {
			int i = 0;
			for (const std::string &column : row) {
				while (column_max_width.size() <= i) column_max_width.emplace_back();
				if (column.length() > column_max_width[i]) column_max_width[i] = column.length();
				i++;
			}
		}

		if (fit_to_width) {
			for (auto &column : column_max_width) {
				column = (width - 2 - 2 * frame_level) / column_max_width.size() - 2;
			}
			column_max_width[column_max_width.size() - 1] -=
				(width - 2 - 2 * frame_level) -
				(width - 2 - 2 * frame_level)
			/ column_max_width.size() * column_max_width.size();
		}

		if (framed) {
			std::cout << "┃";
			std::cout << std::string{"│"} * frame_level;
			std::cout << "┌";
			unsigned long i = column_max_width.size() - 1;
			unsigned long total_width = width - 2;
			for (const unsigned long &length: column_max_width) {
				std::cout << std::string{"─"} * (length + (i ? 2 : 0));
				total_width -= length + (i ? 3 : 0);
				if (i--) std::cout << "┬";
			}
			total_width -= frame_level * 2;
			if (!fit_to_width) std::cout << std::string{"─"} * total_width;
			std::cout << "┐";
			std::cout << std::string{"│"} * frame_level;
			std::cout << "┃\n";
			frame_level++;
		}

		for (const std::vector<std::string> &row : columns) {
			std::string row_string;
			bool first = true;
			int i = 0;
			for (const std::string& column : row) {
				if (!first) row_string += " ";
				if (!first && framed) row_string += std::string{"│ "};
				first = false;
				row_string += set_length(column, column_max_width[i++], "-");
			}
			print(row_string);
		}
		columns.clear();

		if (framed) {
			frame_level--;
			std::cout << "┃";
			std::cout << std::string{"│"} * frame_level;
			std::cout << "└";
			unsigned long i = column_max_width.size() - 1;
			unsigned long total_width = width - 2;
			for (const unsigned long &length: column_max_width) {
				std::cout << std::string{"─"} * (length + (i ? 2 : 0));
				total_width -= length + (i ? 3 : 0);
				if (i--) std::cout << "┴";
			}
			total_width -= frame_level * 2;
			if (!fit_to_width) std::cout << std::string{"─"} * total_width;
			std::cout << "┘";
			std::cout << std::string{"│"} * frame_level;
			std::cout << "┃\n";
		}
	}

	inline void begin_column() {
		current_column++;
		current_row = -1;
	}

	inline void vertical_print(const std::string& string) {
		for (const char &e: string) {
			print({&e, 1});
		}
	}
}
