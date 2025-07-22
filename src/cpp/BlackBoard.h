#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>

class BlackBoard {
public:
	struct Pixel {
		std::string character;
	};
private:
	std::vector<std::vector<Pixel>> board;

	unsigned long width;
	unsigned long height;

public:
	explicit BlackBoard(unsigned long _width = 128, unsigned long _height = -1);
	BlackBoard(std::vector<std::vector<Pixel>>, unsigned long, unsigned long) = delete;

	void flush();

	Pixel& operator[](unsigned long x, unsigned long y);
	Pixel& operator[](const glm::u64vec2& pos);

	void set(const glm::u64vec2& pos, const Pixel& pixel = {" "});

	void rectangle_frame(const glm::u64vec2 & a, const glm::u64vec2 & b, const Pixel &pixel);
	void rectangle_filled(glm::u64vec2 a, glm::u64vec2 b, const Pixel &pixel);

	void rectangle_nice_frame(const glm::u64vec2 & a, const glm::u64vec2 & b, bool bold = false);
};
