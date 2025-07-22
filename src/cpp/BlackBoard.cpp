#include "BlackBoard.h"

#include <iostream>
#include <stdexcept>
#include <bits/unique_ptr.h>
#include <glm/common.hpp>
#include <glm/glm.hpp>

BlackBoard::BlackBoard(
	const unsigned long _width,
	const unsigned long _height
):
	board(_height == -1UL ? 0 : _height),
	width(_width),
	height(_height)
{

}

void BlackBoard::flush() {
	for (const auto& row : board) {
		for (const auto&[character] : row) {
			std::cout << character;
		}
		std::cout << "\n";
	}
	board.clear();
	if (height != -1UL) board.resize(height);

	for (auto& row : board) {
		if (width != -1UL) row.resize(width);
	}
}

BlackBoard::Pixel & BlackBoard::operator[](const unsigned long x, const unsigned long y) {
	if (x > height) throw std::out_of_range("Parameter X greater than height!");
	if (y > width)  throw std::out_of_range("Parameter Y greater than width!");

	return board[x][y];
}

BlackBoard::Pixel & BlackBoard::operator[](const glm::u64vec2& pos) {
	return operator[](pos.x, pos.y);
}

void BlackBoard::set(const glm::u64vec2 &pos, const Pixel &pixel) {
	while (pos.x >= board.size()) board.emplace_back();
	while (pos.y >= board[pos.x].size()) board[pos.x].emplace_back(Pixel{" "});
	board[pos.x][pos.y] = pixel;
}

void BlackBoard::rectangle_frame(const glm::u64vec2 &a, const glm::u64vec2 &b, const Pixel& pixel) {
	glm::u64vec2 pos = a;

	if (pos.x < b.x) {
		while (pos.x < b.x) {
			pos.x++;
			set(pos, pixel);
		}
	} else if (pos.x > b.x) {
		while (pos.x > b.x) {
			pos.x--;
			set(pos, pixel);
		}
	}

	if (pos.y < b.y) {
		while (pos.y < b.y) {
			pos.y++;
			set(pos, pixel);
		}
	} else if (pos.y > b.y) {
		while (pos.y > b.y) {
			pos.y--;
			set(pos, pixel);
		}
	}

	if (pos.x < a.x) {
		while (pos.x < a.x) {
			pos.x++;
			set(pos, pixel);
		}
	} else if (pos.x > a.x) {
		while (pos.x > a.x) {
			pos.x--;
			set(pos, pixel);
		}
	}

	if (pos.y < a.y) {
		while (pos.y < a.y) {
			pos.y++;
			set(pos, pixel);
		}
	} else if (pos.y > a.y) {
		while (pos.y > a.y) {
			pos.y--;
			set(pos, pixel);
		}
	}
}

void BlackBoard::rectangle_filled(glm::u64vec2 a, glm::u64vec2 b, const Pixel &pixel) {
	if (a.x > b.x) std::swap(a.x, b.x);
	if (a.y > b.y) std::swap(a.y, b.y);

	for (unsigned long i = a.x; i < b.x; i++) {
		for (unsigned long j = a.y; j < b.y; j++) {
			set({i, j}, pixel);
		}
	}
}

void BlackBoard::rectangle_nice_frame(const glm::u64vec2 &a, const glm::u64vec2 &b, bool bold) {
	glm::u64vec2 pos = a;

	Pixel pixel = {bold ? "┃" : "│"};

	if (pos.x < b.x) {
		while (pos.x < b.x) {
			pos.x++;
			set(pos, pixel);
		}
	} else if (pos.x > b.x) {
		while (pos.x > b.x) {
			pos.x--;
			set(pos, pixel);
		}
	}

	pixel = {bold ? "━" : "─"};

	if (pos.y < b.y) {
		while (pos.y < b.y) {
			pos.y++;
			set(pos, pixel);
		}
	} else if (pos.y > b.y) {
		while (pos.y > b.y) {
			pos.y--;
			set(pos, pixel);
		}
	}

	pixel = {bold ? "┃" : "│"};

	if (pos.x < a.x) {
		while (pos.x < a.x) {
			pos.x++;
			set(pos, pixel);
		}
	} else if (pos.x > a.x) {
		while (pos.x > a.x) {
			pos.x--;
			set(pos, pixel);
		}
	}

	pixel = {bold ? "━" : "─"};

	if (pos.y < a.y) {
		while (pos.y < a.y) {
			pos.y++;
			set(pos, pixel);
		}
	} else if (pos.y > a.y) {
		while (pos.y > a.y) {
			pos.y--;
			set(pos, pixel);
		}
	}

	set(a, {bold ? "┏" : "┌"});
	set({a.x, b.y}, {bold ? "┓" : "┐"});
	set(b, {bold ? "┛" : "┘"});
	set({b.x, a.y}, {bold ? "┗" : "└"});
}
