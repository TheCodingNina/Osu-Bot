#pragma once

class vec2f {
public:
	// Constructor
	float x, y;
	vec2f(float x, float y) : x(x), y(y) {}
	vec2f() : x(0.f), y(0.f) {}

	// Operators:
	vec2f nor() {
		auto nx = -y, ny = x;
		x = nx;
		y = ny;
		return *this;
	}
	vec2f midPoint(const vec2f &vec) const {
		return vec2f((x + vec.x) / 2.f, (y + vec.y) / 2.f);
	}
	vec2f add(vec2f vec) {
		x += vec.x;
		y += vec.y;
		return *this;
	}
	vec2f add(float ax, float ay) {
		x += ax;
		y += ay;
		return *this;
	}
	vec2f operator+=(const vec2f &alt) {
		x += alt.x;
		y += alt.y;
		return *this;
	}
	vec2f sub(vec2f vec) {
		x -= vec.x;
		y -= vec.y;
		return *this;
	}
	vec2f mult(float n) {
		x *= n;
		y *= n;
		return *this;
	}
	vec2f dev(float n) {
		x /= n;
		y /= n;
		return *this;
	}
	vec2f cpy() const {
		return vec2f(x, y);
	}
	vec2f rotate(float angle) {
		auto nx = x, ny = y;
		x = (nx * cos(angle) - ny * sin(angle));
		y = (nx * sin(angle) + ny * cos(angle));
		return *this;
	}
	float length() const {
		return sqrtf(x * x + y * y);
	}
	vec2f normalize() {
		return this->dev(this->length());
	}
	// Operators END;

	// Destructor
	~vec2f() {}
};

// Inline operators:
inline vec2f operator + (vec2f a, vec2f b) {
	return a.add(b);
}
inline vec2f operator - (vec2f a, vec2f b) {
	return a.sub(b);
}
inline vec2f operator * (vec2f vec, float n) {
	return vec.mult(n);
}
inline vec2f operator * (float n, vec2f vec) {
	return vec.mult(n);
}
inline vec2f operator / (vec2f vec, float n) {
	return vec.dev(n);
}
inline vec2f rotate(vec2f vec, float angle) {
	return vec.rotate(angle);
}
inline float vectorAngle(vec2f vec1, vec2f vec2) {
	return abs(atan2(vec1.x * vec2.y - vec2.x * vec1.y, vec1.x * vec2.y + vec1.y * vec2.x));
}
inline bool operator == (vec2f a, vec2f b) {
	return a.x == b.x && a.y == b.y;
}