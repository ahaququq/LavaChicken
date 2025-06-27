#pragma once
#include <cmath>
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
		if (in.length() <= n) return in + padding * (n - in.length());
		return std::string{in.begin(), in.begin() + n};
	}


	inline unsigned long width = 0;
	inline unsigned long frame_level = 0;

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
		std::cout << "┃";
		std::cout << std::string{"│"} * frame_level;
		std::cout << " " << set_length(string, width - frame_level * 2 - 2) << " ";
		std::cout << std::string{"│"} * frame_level;
		std::cout << "┃\n";
	}

	inline void begin_frame(const std::string &contents) {
		std::cout << "┃";
		std::cout << std::string{"│"} * frame_level;
		std::cout << "┌─ " << set_length(contents, width - 5 - frame_level * 2, "─", true) << "─┐";
		std::cout << std::string{"│"} * frame_level;
		std::cout << "┃\n";
		frame_level++;
	}

	inline void end_frame() {
		if (frame_level == 0) return;
		frame_level--;

		std::cout << "┃";
		std::cout << std::string{"│"} * frame_level;
		std::cout << "└" << std::string{"─"} * (width - 2) << "┘";
		std::cout << std::string{"│"} * frame_level;
		std::cout << "┃\n";
	}

	inline void end() {
		std::cout << "┗" << std::string{"━"} * width << "┛\n";
		frame_level = 0;
	}
}
